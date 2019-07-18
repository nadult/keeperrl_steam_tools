#include <stdio.h>
#include <unistd.h>

#include "keeperrl/directory_path.h"
#include "keeperrl/file_path.h"

#include "steam_friends.h"
#include "steam_client.h"
#include "steam_ugc.h"
#include "steam_utils.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

void printFriends(steam::Client& client) {
  auto& friends = client.friends();

  auto ids = friends.ids();
  for (int n = 0; n < ids.size(); n++)
    printf("Friend #%d: %s [%llu]\n", n, friends.name(ids[n]).c_str(), ids[n].ConvertToUint64());
  fflush(stdout);
}

void displayImage(vector<uint8_t> data, pair<int, int> size) {
  int max_size = 16, sub_size = 2;
  CHECK(size.first == max_size * sub_size && size.second == size.first);

  for (int y = 0; y < max_size; y++) {
    for (int x = 0; x < max_size; x++) {
      int value = 0;
      for (int ix = 0; ix < sub_size; ix++)
        for (int iy = 0; iy < sub_size; iy++)
          value += data[(x * sub_size + ix + (y * sub_size + iy) * size.first) * 4 + 1];
      value /= sub_size * sub_size;
      printf("%c", value > 80 ? 'X' : ' ');
    }
    printf("\n");
  }
  fflush(stdout);
}

void printFriendAvatars(steam::Client& client) {
  auto& friends = client.friends();
  auto& utils = client.utils();

  auto ids = friends.ids();
  vector<int> results(ids.size(), -1);
  vector<char> completed(ids.size(), false);

  for (int n = 0; n < ids.size(); n++)
    results[n] = friends.avatar(ids[n], 0);

  int num_completed = 0;
  for (int r = 0; r < 100 && num_completed < ids.size(); r++) {
    for (int n = 0; n < ids.size(); n++) {
      if (completed[n])
        continue;

      results[n] = friends.avatar(ids[n], 0);
      if (results[n] == 0) {
        printf("%d: no avatar\n", n);
        num_completed++;
        completed[n] = true;
      } else if (results[n] == -1) {
        continue;
      } else {
        auto size = utils.imageSize(results[n]);
        printf("%d: Getting avatar data (%dx%d)\n", n, size.first, size.second);
        auto data = utils.imageData(results[n]);
        displayImage(data, size);

        completed[n] = true;
        num_completed++;
      }
    }
    usleep(100 * 1000);
  }
  printf("Completed: %d\n", num_completed);
}

void printIndent(int size) {
  for (int n = 0; n < size; n++)
    printf(" ");
}

void printFiles(string path_str, int indent, bool with_subdirs) {
  DirectoryPath path(path_str);
  auto files = path.getFiles();
  auto dirs = path.getSubDirs();

  for (auto& dir : dirs) {
    printIndent(indent);
    printf("%s/\n", dir.c_str());
    if (with_subdirs)
      printFiles(path_str + "/" + dir, indent + 2, with_subdirs);
  }
  for (auto& file : files) {
    printIndent(indent);
    auto fname = split(file.getPath(), {'/'}).back();
    printf("%s\n", fname.c_str());
  }
}

string shortenDesc(string desc, int max_len = 60) {
  if ((int)desc.size() > max_len)
    desc.resize(max_len);
  auto it = desc.find("\n");
  if (it != string::npos)
    desc.resize(it);
  return desc;
}

void printQueryInfo(steam::UGC& ugc, steam::UGC::QueryId qid) {
  auto& info = ugc.queryInfo(qid);
  auto status = ugc.queryStatus(qid);

  if (status == QueryStatus::completed) {
    auto results = ugc.queryResults(qid);
    printf("Query completed with %d / %d results\n", results.count, results.total);

    for (int n = 0; n < results.count; n++) {
      auto details = ugc.queryDetails(qid, n);
      printf("  %d: %s\n", n, details.m_rgchTitle);
      string desc = shortenDesc(details.m_rgchDescription);
      printf("  desc: %s\n\n", desc.c_str());
    }
  } else if (status == QueryStatus::failed) {
    printf("Query failed: %s\n", ugc.queryError(qid).c_str());
  }
}

void printWorkshopItems(steam::Client& client) {
  auto& ugc = client.ugc();

  auto items = ugc.subscribedItems();
  for (auto item : items) {
    auto state = ugc.state(item);
    printf("Item #%d: %s\n", (int)item, steam::itemStateText(state).c_str());
    if (state & k_EItemStateInstalled) {
      auto info = ugc.installInfo(item);
      printf("  Installed at: %s\n  Size: %llu  Time_stamp: %u\n", info.folder.c_str(), info.sizeOnDisk,
             info.timeStamp);
      printFiles(info.folder, 2, false);
      printf("\n");
    }
    if (state & k_EItemStateDownloading) {
      auto info = ugc.downloadInfo(item);
      printf("  Downloading: %llu / %llu bytes\n", info.bytesDownloaded, info.bytesTotal);
    }
  }

  steam::QueryInfo qinfo;
  qinfo.keyValueTags = true;
  qinfo.longDescription = true;
  auto qid = ugc.createQuery(qinfo, items);

  int num_retries = 20;
  for (int r = 0; r < num_retries; r++) {
    steam::runCallbacks();
    ugc.updateQueries();

    if (ugc.queryStatus(qid) != QueryStatus::pending) {
      printQueryInfo(ugc, qid);
      break;
    }
    usleep(100 * 1000);
  }

  if (ugc.queryStatus(qid) == QueryStatus::pending)
    printf("Query takes too long!\n");
  ugc.finishQuery(qid);
}

void printAllWorkshopItems(steam::Client& client) {
  auto& ugc = client.ugc();
  auto& utils = client.utils();

  steam::QueryInfo qinfo;
  //qinfo.metadata = true;
  qinfo.onlyIds = true;
  qinfo.keyValueTags = true;
  auto qid = ugc.createQuery(qinfo, k_EUGCQuery_RankedByVote, k_EUGCMatchingUGCType_All, utils.appId(), 1);

  int num_retries = 20;
  for (int r = 0; r < num_retries; r++) {
    steam::runCallbacks();
    ugc.updateQueries();

    if (ugc.queryStatus(qid) != QueryStatus::pending) {
      printQueryInfo(ugc, qid);
      break;
    }
    usleep(100 * 1000);
  }

  if (ugc.queryStatus(qid) == QueryStatus::pending)
    printf("Query takes too long!\n");
  ugc.finishQuery(qid);
}

struct ModInfo {
  string name;
  string description;

  static optional<ModInfo> load(DirectoryPath folder) {
    if (!folder.exists()) {
      printf("Folder %s does not exist!\n", folder.getPath());
      return none;
    }

    auto file = folder.file("mod_info.txt");
    if (auto contents = file.readContents()) {
      auto lines = split(*contents, {'\n'});
      if (lines.size() < 2) {
        printf("mod_info.txt should contain at least 2 lines:\n-first line: mod name\n-following lines: mod "
               "description\n");
        return none;
      }

      string description;
      for (int n = 1; n < lines.size(); n++)
        description += lines[n] + "\n";
      return ModInfo{lines[0], description};
    } else {
      printf("Missing mod_info.txt file\n");
    }

    return none;
  }
};

void addWorkshopItem(steam::Client& client, optional<unsigned long long> id, const steam::ItemInfo& itemInfo) {
  auto& ugc = client.ugc();
  bool legal = false; // TODO: handle it

  if (!id) {
    ugc.beginCreateItem();
    int num_retries = 100;
    for (int r = 0; r < num_retries; r++) {
      steam::runCallbacks();
      if (auto result = ugc.tryCreateItem()) {
        if (result->valid()) {
          id = result->itemId;
          legal = result->requireLegalAgreement;
        } else {
          printf("Error while creating new workshop item: %s\n", steam::errorText(result->result).c_str());
          return;
        }
        break;
      }
      usleep(100 * 1000);
    }
    if (ugc.isCreatingItem()) {
      printf("Error while creating new workshop item: Query takes too long!\n");
      return;
    }
  }

  ugc.updateItem(itemInfo, *id);
  while (true) {
    steam::runCallbacks();
    if (auto result = ugc.tryUpdateItem()) {
      if (result->valid()) {
        printf("Item %llu added!\n", (unsigned long long)*id);
        legal |= result->requireLegalAgreement;
      } else {
        printf("Error while updating new workshop item: %s\n", steam::errorText(result->result).c_str());
        return;
      }
      break;
    }
    usleep(100 * 1000);
  }
}

void addWorkshopItem(steam::Client& client, optional<unsigned long long> id, string folderName) {
  DirectoryPath folder(folderName);
  auto modInfo = ModInfo::load(folder);
  if (!modInfo)
    return;

  steam::ItemInfo itemInfo;
  itemInfo.description = modInfo->description;
  itemInfo.title = modInfo->name;
  itemInfo.folder = (string)folder.absolute().getPath();
  // TODO: preview
  itemInfo.version = 28; // TODO
  itemInfo.visibility = k_ERemoteStoragePublishedFileVisibilityPrivate; //TODO
  addWorkshopItem(client, id, itemInfo);
}

void updateWorkshopPreview(steam::Client& client, unsigned long long id, string fileName) {
  if (!isAbsolutePath(fileName.c_str()))
    fileName = string(DirectoryPath::current().getPath()) + "/" + fileName;
  auto filePath = FilePath::fromFullPath(fileName);
  if (!filePath.exists()) {
    printf("File %s does not exist!\n", fileName.c_str());
    return;
  }

  steam::ItemInfo itemInfo;
  itemInfo.preview = (string)filePath.getPath();
  addWorkshopItem(client, id, itemInfo);
}

void printHelp() {
  printf("Options:\n-help\n-friends\n-avatars\n-workshop\n-full-workshop\n-add-workshop-item "
         "[folder]\n-update-workshop-item [id] [folder]\n-update-workshop-preview [id] [image]\n");
}

int main(int argc, char** argv) {
  if (argc <= 1) {
    printHelp();
    return 0;
  }

  FatalLog.addOutput(DebugOutput::crash());
  FatalLog.addOutput(DebugOutput::toStream(std::cerr));
  InfoLog.addOutput(DebugOutput::toStream(std::cerr));

  if (!steam::initAPI()) {
    printf("Steam is not running\n");
    return 0;
  }

  steam::Client client;
  client.numberOfCurrentPlayers();

  for (int n = 1; n < argc; n++) {
    string option = argv[n];

    if (option == "-friends")
      printFriends(client);
    else if (option == "-avatars")
      printFriendAvatars(client);
    else if (option == "-workshop")
      printWorkshopItems(client);
    else if (option == "-full-workshop")
      printAllWorkshopItems(client);
    else if (option == "-add-workshop-item") {
      CHECK(n + 1 < argc);
      auto folder = argv[++n];
      addWorkshopItem(client, none, folder);
    } else if (option == "-update-workshop-item") {
      CHECK(n + 2 < argc);
      auto id = atoll(argv[++n]);
      CHECK(id > 0);
      auto folder = argv[++n];
      addWorkshopItem(client, id, folder);
    } else if (option == "-update-workshop-preview") {
      CHECK(n + 2 < argc);
      auto id = atoll(argv[++n]);
      CHECK(id > 0);
      auto fileName = argv[++n];
      updateWorkshopPreview(client, id, fileName);
    } else if (option == "-help")
      printHelp();
    else {
      printf("unknown option: %s\n", argv[n]);
      return 0;
    }
  }

  steam::runCallbacks();

  if (auto nocp = client.numberOfCurrentPlayers())
    printf("Number of players: %d\n", *nocp);
  else
    printf("Couldn't get nr of players...\n");

  return 0;
}
