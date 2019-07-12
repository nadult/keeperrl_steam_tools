#include <stdio.h>
#include <unistd.h>

#include "keeperrl/directory_path.h"
#include "keeperrl/file_path.h"

#include "steam_friends.h"
#include "steam_client.h"
#include "steam_ugc.h"
#include "steam_utils.h"

void printFriends(const steam::Client& client) {
  auto friends = client.friends();

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

void printFriendAvatars(const steam::Client& client) {
  auto friends = client.friends();
  auto utils = client.utils();

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

template <class T, int size> constexpr int arraySize(T (&)[size]) {
  return size;
}

string itemStateText(uint32_t bits) {
  static const char* names[] = {"subscribed",   "legacy_item", "installed",
                                "needs_update", "downloading", "download_pending"};

  if (bits == k_EItemStateNone)
    return "none";

  string out;
  for (int n = 0; n < arraySize(names); n++)
    if (bits & (1 << n)) {
      if (!out.empty())
        out += ' ';
      out += names[n];
    }
  return out;
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
    printf("Query failed: %s\n", ugc.queryError(qid));
  }
}

void printWorkshopItems(steam::Client& client) {
  auto& ugc = client.ugc();
  auto utils = client.utils();

  auto items = ugc.subscribedItems();
  for (auto item : items) {
    auto state = ugc.state(item);
    printf("Item #%d: %s\n", (int)item, itemStateText(state).c_str());
    if (state & k_EItemStateInstalled) {
      auto info = ugc.installInfo(item);
      printf("  Installed at: %s\n  Size: %llu  Time_stamp: %u\n", info.folder.c_str(), info.size_on_disk,
             info.time_stamp);
      printFiles(info.folder, 2, false);
      printf("\n");
    }
    if (state & k_EItemStateDownloading) {
      auto info = ugc.downloadInfo(item);
      printf("  Downloading: %llu / %llu bytes\n", info.bytes_downloaded, info.bytes_total);
    }
  }

  steam::QueryInfo qinfo;
  qinfo.key_value_tags = true;
  qinfo.long_description = true;
  auto qid = ugc.createQuery(qinfo, items);

  int num_retries = 20;
  for (int r = 0; r < num_retries; r++) {
    steam::runCallbacks();
    ugc.updateQueries(utils);

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
  auto utils = client.utils();

  steam::QueryInfo qinfo;
  //qinfo.metadata = true;
  qinfo.only_ids = true;
  qinfo.key_value_tags = true;
  auto qid = ugc.createQuery(qinfo, k_EUGCQuery_RankedByVote, k_EUGCMatchingUGCType_All, utils.appId(), 1);

  int num_retries = 20;
  for (int r = 0; r < num_retries; r++) {
    steam::runCallbacks();
    ugc.updateQueries(utils);

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

void printHelp() {
  printf("Options:\n-help\n-friends\n-avatars\n-workshop\n-full-workshop\n");
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
    else if (option == "-help")
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
