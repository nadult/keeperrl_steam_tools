#include <stdio.h>
#include <unistd.h>

#include "directory_path.h"
#include "file_path.h"
#include "steam_friends.h"
#include "steam_client.h"
#include "steam_ugc.h"
#include "steam_utils.h"

#define TEXT InfoLog.get()

/*
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
}*/

string shortenDesc(string desc, int maxLen = 60) {
  maxLen -= 3;
  bool shortened = false;
  if ((int)desc.size() > maxLen) {
    desc.resize(maxLen);
    shortened = true;
  }

  auto pos = desc.find("\n");
  if (pos != string::npos) {
    shortened |= desc.size() > pos;
    desc.resize(pos);
  }

  if (shortened)
    desc += "...";
  return desc;
}

void handleQueryError(steam::UGC& ugc, int qid) {
  if (ugc.queryStatus(qid) != QueryStatus::completed) {
    auto error = ugc.queryError(qid, "Query took too much time");
    TEXT << error;
    ugc.finishQuery(qid);
    exit(1);
  }
}

void printItemsInfo(steam::Client& client, const vector<steam::ItemId>& items) {
  auto& ugc = client.ugc();
  auto& utils = client.utils();
  auto& friends = client.friends();

  steam::ItemDetailsInfo qinfo;
  qinfo.keyValueTags = true;
  qinfo.longDescription = true;
  qinfo.metadata = true;
  qinfo.playtimeStatsDays = 5000;
  auto qid = ugc.createDetailsQuery(qinfo, items);

  ugc.waitForQueries({qid}, 100);
  handleQueryError(ugc, qid);

  auto infos = ugc.finishDetailsQuery(qid);
  vector<optional<string>> ownerNames(infos.size());

  for (auto& info : infos)
    friends.requestUserInfo(info.ownerId, true);

  auto retrieveUserNames = [&]() {
    bool done = true;
    for (int n = 0; n < infos.size(); n++) {
      if (!ownerNames[n])
        ownerNames[n] = friends.retrieveUserName(infos[n].ownerId);
      if (!ownerNames[n])
        done = false;
    }
    return done;
  };

  steam::sleepUntil(retrieveUserNames, 40);

  for (int n = 0; n < infos.size(); n++) {
    auto& info = infos[n];
    string ownerName = ownerNames[n].value_or("?");

    TEXT << "Item #" << info.id << " ----------------------";
    TEXT << "       title: " << info.title;
    TEXT << " description: " << shortenDesc(info.description);
    TEXT << "       owner: " << ownerName << " [" << info.ownerId.ConvertToUint64() << "]";
    TEXT << "       score: " << info.score << "(+" << info.votesUp << " / -" << info.votesDown << ")";
    TEXT << "        tags: " << info.tags;
    TEXT << "       stats: " << info.stats->subscriptions << " subscriptions, "
                             << info.stats->followers << " followers, "
                             << info.stats->favorites << " favorites";
    TEXT << "              " << info.stats->secondsPlayed / 3600 << " hours played, "
                             << info.stats->playtimeSessions << " times played";
    TEXT << "              " << info.stats->comments << " comments, "
                             << info.stats->uniqueWebsiteViews << " website views";
                                
    TEXT << "";
  }
}

void updateItem(steam::Client& client, const steam::UpdateItemInfo& itemInfo) {
  auto& ugc = client.ugc();
  bool legal = false; // TODO: handle it

  ugc.beginUpdateItem(itemInfo);

  // Item update may take some time; Should we loop indefinitely?
  optional<steam::UpdateItemResult> result;
  steam::sleepUntil([&]() { return (bool)(result = ugc.tryUpdateItem()); }, 60 * 10, milliseconds(1000));

  if (!result) {
    TEXT << "Timeout!\n";
    ugc.cancelUpdateItem();
    return;
  }

  if (result->valid()) {
    TEXT << "Item " << (itemInfo.id ? "created" : "updated") << "!";
    if (!itemInfo.id)
      TEXT << "ID: " << *result->itemId;
    legal |= result->requireLegalAgreement;
  } else {
    TEXT << *result->error;

    // Remove partially created item
    if (!itemInfo.id && result->itemId)
      ugc.deleteItem(*result->itemId);
  }
}

void findItems(steam::Client& client, const steam::FindItemInfo& findInfo) {
  auto& ugc = client.ugc();
  auto maxCount = findInfo.maxItemCount.value_or(INT_MAX);

  int pageId = 1;
  int itemCount = 0;

  while (itemCount < maxCount) {
    auto qid = ugc.createFindQuery(findInfo, pageId++);
    ugc.waitForQueries({qid}, 50);
    handleQueryError(ugc, qid);

    auto ids = ugc.finishFindQuery(qid);
    if (ids.empty())
      break;

    qid = ugc.createDetailsQuery({}, ids);
    ugc.waitForQueries({qid}, 50);
    handleQueryError(ugc, qid);

    auto infos = ugc.finishDetailsQuery(qid);
    for (int n = 0; n < ids.size(); n++)
      printf("%12llu  %s\n", (unsigned long long)ids[n], infos[n].title.c_str());
    itemCount += ids.size();
  }

  TEXT << "\n" << itemCount << " items found.";
}

struct Option {
  string name, value;
};

vector<Option> parseOptions(int argc, char** argv) {
  vector<Option> out;

  auto checkName = [](const string& name) {
    for (auto c : name)
      if (isspace(c))
        FATAL << "Invalid command name: '" << name << "'";
  };

  for (int n = 1; n < argc; n++) {
    if (auto separator = strchr(argv[n], '=')) {
      string name(argv[n], separator);
      checkName(name);
      string value(separator + 1);
      out.emplace_back(Option{name, value});
    } else {
      string name(argv[n]);
      checkName(name);
      out.emplace_back(Option{name, {}});
    }
  }

  return out;
}

string parseCommand(vector<Option>& options) {
  string command = "help";

  if (!options.empty()) {
    if (!options[0].value.empty())
      FATAL << "Command shouldn't have a value specified";
    command = options[0].name;
    options.removeIndexPreserveOrder(0);
  }
  return command;
}

unsigned long long parseId(const string& value) {
  auto id = atoll(value.c_str());
  if (id <= 0)
    FATAL << "Invalid ID specified";
  return id;
}

string parseAbsoluteFilePath(string fileName) {
  if (!isAbsolutePath(fileName.c_str()))
    fileName = string(DirectoryPath::current().getPath()) + "/" + fileName;
  auto filePath = FilePath::fromFullPath(fileName);
  if (!filePath.exists())
    FATAL << "File '" << fileName << "' does not exist!";
  return filePath.getPath();
}

string parseAbsoluteFolderPath(const string& value) {
  auto path = DirectoryPath(value).absolute();
  if (!path.exists())
    FATAL << "Folder '" << path.getPath() << "' does not exist!";
  return path.getPath();
}

string loadFileContents(const string& fileName) {
  auto filePath = FilePath::fromFullPath(fileName);
  if (!filePath.exists())
    FATAL << "File '" << fileName << "' does not exist!";
  auto contents = filePath.readContents();
  if (!contents)
    FATAL << "Error while reading file '" << fileName << "'";
  return *contents;
}

auto parseVisibility(const string& value) {
  using Vis = SteamItemVisibility;
  if (value == "public")
    return Vis::public_;
  else if (value == "private")
    return Vis::private_;
  else if (value == "friends")
    return Vis::friends;
  FATAL << "Invalid visibility value: '" << value << "'";
  return Vis::private_;
}

void printValidTags();

string parseTag(const string& value) {
  for (auto tag : steam::validTags())
    if (value == tag)
      return value;
  printValidTags();
  FATAL << "Invalid tag specified: '" << value << "'";
  return {};
}

steam::UpdateItemInfo parseItemInfo(const vector<Option>& options) {
  steam::UpdateItemInfo out;

  for (auto& option : options) {
    if (option.name == "id")
      out.id = parseId(option.value);
    else if (option.name == "title")
      out.title = option.value; // TODO: validate?
    else if (option.name == "folder")
      out.folder = parseAbsoluteFolderPath(option.value);
    else if (option.name == "preview")
      out.previewFile = parseAbsoluteFilePath(option.value);
    else if (option.name == "tag") {
      if (!out.tags)
        out.tags = vector<string>();
      out.tags->emplace_back(parseTag(option.value));
    } else if (option.name == "remove-tags")
      out.tags = vector<string>();
    else if (option.name == "desc")
      out.description = loadFileContents(option.value);
    else if (option.name == "visibility")
      out.visibility = parseVisibility(option.value);
  }
  return out;
}

vector<steam::ItemId> parseItemIds(const vector<Option>& options) {
  vector<steam::ItemId> out;
  for (auto& option : options) {
    if (option.name == "id")
      out.emplace_back(parseId(option.value));
    else
      TEXT << "Ignored option: " << option.name;
  }
  return out;
}

steam::FindOrder parseFindOrder(const string& value) {
  if (auto order = EnumInfo<steam::FindOrder>::fromStringSafe(value))
    return *order;
  FATAL << "Invalid order specified: " << value;
  return {};
}

int parseItemCount(const string& value) {
  auto count = atoi(value.c_str());
  if (count <= 0)
    FATAL << "Invalid value specified";
  return count;
}

auto parseFindInfo(const vector<Option>& options) {
  steam::FindItemInfo out;
  for (auto& option : options) {
    if (option.name == "phrase")
      out.searchText = option.value;
    else if (option.name == "tags")
      out.tags = option.value;
    else if (option.name == "any-tag")
      out.anyTag = true;
    else if (option.name == "order")
      out.order = parseFindOrder(option.value);
    else if (option.name == "max-count")
      out.maxItemCount = parseItemCount(option.value);
    else
      TEXT << "Ignored option: " << option.name;
  }
  return out;
}

void printHelp() {
  TEXT << "Commands:\n"
          "  help      print this help\n"
          "  help-tags print list of valid tags\n"
          "  update    update workshop item\n"
          "  add       add new workshop item\n"
          "  find      look for workshop items\n"
          "  info      prinf information about workshop items\n"
          "            multiple id's can be passed\n"
          "\n"
          "Add / Update options:\n"
          "  id={}         unique identifier (the same id which is in steamcommunity.com URL)\n"
          "  title={}      specify new title\n"
          "  folder={}     specify folder with mod contents\n"
          "  preview={}    specify file with preview image\n"
          "  tag={}        specify tag (you can pass multiple)\n"
          "  remove-tags   remove all tags\n"
          "  desc={}       specify file with item description\n"
          "  visibility={} public, friends or private\n"
          "\n"
          "Find options:\n"
          "  phrase={}    fitler mods by specified phrase (from title or description)\n"
          "  tags={}      list of tags separated by comma; mod will have to match all of them\n"
          "               unless 'any-tag' option is used\n"
          "  any-tag      return mods which match at least one tag\n"
          "  max-count={} limit nr of items fetched\n"
          "\n"
          "Examples:\n"
          "$ steam_utils add name=\"My new mod\" folder=\"my_new_mod/\" desc=my_new_mod.txt tag=\"Alpha 29\"\n"
          "$ steam_utils update id=1806744451 desc=updated_description.txt\n"
          "$ steam_uptils find tags=\"Alpha 29\"\n";
}

// TODO: tabki trzeba najpierw włączyć na steam partnerze
void printValidTags() {
  TEXT << "Valid tags:";
  for (auto tag : steam::validTags())
    TEXT << "'" << tag << "'";
  TEXT << "\n";
}

int main(int argc, char** argv) {
  FatalLog.addOutput(DebugOutput::crash());
  FatalLog.addOutput(DebugOutput::toStream(std::cerr));
  InfoLog.addOutput(DebugOutput::toStream(std::cerr));

  auto options = parseOptions(argc, argv);
  auto command = parseCommand(options);
  if (command == "help") {
    printHelp();
    return 0;
  } else if (command == "help-tags") {
    printValidTags();
    return 0;
  }

  if (!steam::initAPI()) {
    printf("Steam is not running\n");
    return 1;
  }
  steam::Client client;

  if (command == "add") {
    auto itemInfo = parseItemInfo(options);
    if (!itemInfo.title || !itemInfo.folder)
      FATAL << "When adding item, title and folder have to be specified";
    updateItem(client, itemInfo);
  } else if (command == "update") {
    auto itemInfo = parseItemInfo(options);
    if (!itemInfo.id)
      FATAL << "When updating item, id has to be specified";
    updateItem(client, itemInfo);
  } else if (command == "find") {
    auto findInfo = parseFindInfo(options);
    findItems(client, findInfo);
  } else if (command == "info") {
    auto ids = parseItemIds(options);
    printItemsInfo(client, ids);
  } else {
    TEXT << "Invalid command: " << command;
    return 1;
  }

  return 0;
}
