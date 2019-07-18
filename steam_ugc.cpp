#include "steam_internal.h"
#include "steam_ugc.h"
#include "steam_utils.h"

#define FUNC(name, ...) SteamAPI_ISteamUGC_##name

namespace steam {

UGC::UGC(intptr_t ptr) : ptr(ptr) {
}
UGC::~UGC() = default;

int UGC::numSubscribedItems() const {
  return (int)FUNC(GetNumSubscribedItems)(ptr);
}

vector<ItemId> UGC::subscribedItems() const {
  vector<ItemId> out(numSubscribedItems());
  int result = FUNC(GetSubscribedItems)(ptr, out.data(), out.size());
  out.resize(result);
  return out;
}

uint32_t UGC::state(ItemId id) const {
  return FUNC(GetItemState)(ptr, id);
}

DownloadInfo UGC::downloadInfo(ItemId id) const {
  uint64 downloaded = 0, total = 0;
  auto result = FUNC(GetItemDownloadInfo)(ptr, id, &downloaded, &total);
  // TODO: handle result
  return {downloaded, total};
}

InstallInfo UGC::installInfo(ItemId id) const {
  uint64 size_on_disk;
  uint32 time_stamp;
  char buffer[4096];
  auto result = FUNC(GetItemInstallInfo)(ptr, id, &size_on_disk, buffer, sizeof(buffer) - 1, &time_stamp);
  buffer[sizeof(buffer) - 1] = 0;
  return {size_on_disk, buffer, time_stamp};
}

UGC::QueryId UGC::allocQuery(QHandle handle, const QueryInfo& info) {
  int qid = -1;
  for (int n = 0; n < queries.size(); n++)
    if (!queries[n].valid()) {
      qid = n;
      break;
    }
  if (qid == -1) {
    qid = queries.size();
    queries.emplace_back();
  }

  auto& query = queries[qid];
  query.handle = handle;
  query.info = info;
  return qid;
}

void UGC::setupQuery(QHandle handle, const QueryInfo& info) {
#define SET_VAR(var, func)                                                                                             \
  if (info.var)                                                                                                        \
    FUNC(SetReturn##func)(ptr, handle, true);
  SET_VAR(additionalPreviews, AdditionalPreviews)
  SET_VAR(children, Children)
  SET_VAR(keyValueTags, KeyValueTags)
  SET_VAR(longDescription, LongDescription)
  SET_VAR(metadata, Metadata)
  SET_VAR(onlyIds, OnlyIDs)
  SET_VAR(playtimeStats, PlaytimeStats)
  SET_VAR(totalOnly, TotalOnly)
#undef SET
  if (!info.searchText.empty())
    FUNC(SetSearchText)(ptr, handle, info.searchText.c_str());
}

UGC::QueryId UGC::createQuery(const QueryInfo& info, vector<ItemId> items) {
  CHECK(items.size() >= 1);

  auto handle = FUNC(CreateQueryUGCDetailsRequest)(ptr, items.data(), items.size());
  CHECK(handle != invalidHandle);
  // TODO: properly handle errors

  setupQuery(handle, info);
  auto qid = allocQuery(handle, info);
  queries[qid].items = std::move(items);
  queries[qid].call = FUNC(SendQueryUGCRequest)(ptr, handle);
  return qid;
}

UGC::QueryId UGC::createQuery(const QueryInfo& info, EUGCQuery type, EUGCMatchingUGCType matching_type, unsigned app_id,
                              int page_id) {
  CHECK(page_id >= 1);
  auto handle = FUNC(CreateQueryAllUGCRequest)(ptr, type, matching_type, app_id, app_id, page_id);
  CHECK(handle != k_UGCQueryHandleInvalid);
  // TODO: properly handle errors

  setupQuery(handle, info);

  auto qid = allocQuery(handle, info);
  queries[qid].call = FUNC(SendQueryUGCRequest)(ptr, handle);
  return qid;
}

void UGC::updateQueries() {
  for (int n = 0; n < queries.size(); n++) {
    auto& query = queries[n];
    if (!query.valid())
      continue;

    if (query.call.status == QStatus::pending)
      query.call.update();
  }
}

void UGC::finishQuery(QueryId qid) {
  CHECK(isQueryValid(qid));
  FUNC(ReleaseQueryUGCRequest)(ptr, queries[qid].handle);
  queries[qid].handle = invalidHandle;
}

bool UGC::isQueryValid(QueryId qid) const {
  return queries[qid].valid();
}

QueryStatus UGC::queryStatus(QueryId qid) const {
  CHECK(isQueryValid(qid));
  return queries[qid].call.status;
}

const QueryInfo& UGC::queryInfo(QueryId qid) const {
  CHECK(isQueryValid(qid));
  return queries[qid].info;
}

QueryResults UGC::queryResults(QueryId qid) const {
  CHECK(queryStatus(qid) == QStatus::completed);
  auto& result = queries[qid].call.result();
  return {(int)result.m_unNumResultsReturned, (int)result.m_unTotalMatchingResults};
}

string UGC::queryError(QueryId qid) const {
  CHECK(isQueryValid(qid));
  auto& call = queries[qid].call;
  if (call.status == QStatus::failed)
    return call.failText();
  return "";
}

QueryDetails UGC::queryDetails(QueryId qid, int index) {
  CHECK(queryStatus(qid) == QStatus::completed);
  auto& query = queries[qid];

  QueryDetails out;
  auto result = FUNC(GetQueryUGCResult)(ptr, query.handle, index, &out);
  CHECK(result);
  // TODO: properly handle errors
  return out;
}

string UGC::queryMetadata(QueryId qid, int index) {
  CHECK(queryStatus(qid) == QStatus::completed);
  auto& query = queries[qid];
  CHECK(query.info.metadata);

  char buffer[4096];
  auto result = FUNC(GetQueryUGCMetadata)(ptr, query.handle, index, buffer, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = 0;
  CHECK(result);
  return buffer;
}

vector<pair<string, string>> UGC::queryKeyValueTags(QueryId qid, int index) {
  vector<pair<string, string>> out;

  CHECK(queryStatus(qid) == QStatus::completed);
  auto& query = queries[qid];
  CHECK(query.info.keyValueTags);

  char buf1[4096], buf2[4096];
  auto count = FUNC(GetQueryUGCNumKeyValueTags)(ptr, query.handle, index);

  out.resize(count);
  for (unsigned n = 0; n < count; n++) {
    auto result =
        FUNC(GetQueryUGCKeyValueTag)(ptr, query.handle, index, n, buf1, sizeof(buf1) - 1, buf2, sizeof(buf2) - 1);
    CHECK(result);
    buf1[sizeof(buf1) - 1] = 0;
    buf2[sizeof(buf2) - 1] = 0;
    out[n] = make_pair(buf1, buf2);
  }
  return out;
}

void UGC::beginCreateItem() {
  CHECK(!isCreatingItem());
  auto appId = Utils::instance().appId();
  createItemQuery = FUNC(CreateItem)(ptr, appId, k_EWorkshopFileTypeCommunity);
}

optional<CreateItemInfo> UGC::tryCreateItem() {
  createItemQuery.update();
  if (createItemQuery.isCompleted()) {
    auto out = createItemQuery.result();
    createItemQuery.clear();
    return out;
  }
  return none;
}

bool UGC::isCreatingItem() const {
  return !!createItemQuery;
}

void UGC::cancelCreateItem() {
  createItemQuery.clear();
}

void UGC::updateItem(const ItemInfo& info, ItemId id) {
  printf("updating item: %llu %s\nFolder: %s\n", (unsigned long long)id, info.title.c_str(), info.folder.c_str());

  auto appId = Utils::instance().appId();
  auto handle = FUNC(StartItemUpdate)(ptr, appId, id);

  FUNC(SetItemTitle)(ptr, handle, info.title.c_str());
  FUNC(SetItemDescription)(ptr, handle, info.description.c_str());
  FUNC(SetItemContent)(ptr, handle, info.folder.c_str());
  FUNC(SetItemVisibility)(ptr, handle, info.visibility);
  // TODO: version

  updateItemQuery = FUNC(SubmitItemUpdate)(ptr, handle, nullptr);
}

optional<UpdateItemInfo> UGC::tryUpdateItem() {
  updateItemQuery.update();
  if (updateItemQuery.isCompleted()) {
    auto out = updateItemQuery.result();
    updateItemQuery.clear();
    return out;
  }
  return none;
}

bool UGC::isUpdatingItem() {
  return !!updateItemQuery;
}

void UGC::cancelUpdateItem() {
  updateItemQuery.clear();
}

void UGC::updatePreview(const string& fileName, ItemId id) {
  auto appId = Utils::instance().appId();
  auto handle = FUNC(StartItemUpdate)(ptr, appId, id);

  FUNC(SetItemPreview)(ptr, handle, fileName.c_str());
  updatePreviewQuery = FUNC(SubmitItemUpdate)(ptr, handle, nullptr);
}

optional<UpdateItemInfo> UGC::tryUpdatePreview() {
  updatePreviewQuery.update();
  if (updatePreviewQuery.isCompleted()) {
    auto out = updatePreviewQuery.result();
    updatePreviewQuery.clear();
    return out;
  }
  return none;
}

bool UGC::isUpdatingPreview() {
  return !!updatePreviewQuery;
}

void UGC::cancelUpdatePreview() {
  updatePreviewQuery.clear();
}
}
