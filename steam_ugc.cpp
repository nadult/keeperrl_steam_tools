#include "steam_internal.h"
#include "steam_ugc.h"
#include "steam_utils.h"

#define FUNC(name, ...) SteamAPI_ISteamUGC_##name

namespace steam {

UGC::UGC(intptr_t ptr) : m_ptr(ptr) {
}

UGC::~UGC() {
}

int UGC::numSubscribedItems() const {
  return (int)FUNC(GetNumSubscribedItems)(m_ptr);
}

vector<ItemId> UGC::subscribedItems() const {
  vector<ItemId> out(numSubscribedItems());
  int result = FUNC(GetSubscribedItems)(m_ptr, out.data(), out.size());
  out.resize(result);
  return out;
}

uint32_t UGC::state(ItemId id) const {
  return FUNC(GetItemState)(m_ptr, id);
}

DownloadInfo UGC::downloadInfo(ItemId id) const {
  uint64 downloaded = 0, total = 0;
  auto result = FUNC(GetItemDownloadInfo)(m_ptr, id, &downloaded, &total);
  // TODO: handle result
  return {downloaded, total};
}

InstallInfo UGC::installInfo(ItemId id) const {
  uint64 size_on_disk;
  uint32 time_stamp;
  char buffer[4096];
  auto result = FUNC(GetItemInstallInfo)(m_ptr, id, &size_on_disk, buffer, sizeof(buffer) - 1, &time_stamp);
  buffer[sizeof(buffer) - 1] = 0;
  return {size_on_disk, buffer, time_stamp};
}

UGC::QueryId UGC::allocQuery(QHandle handle, const QueryInfo& info) {
  int qid = -1;
  for (int n = 0; n < m_queries.size(); n++)
    if (!m_queries[n].valid()) {
      qid = n;
      break;
    }
  if (qid == -1) {
    qid = m_queries.size();
    m_queries.emplace_back();
  }

  auto& query = m_queries[qid];
  query.handle = handle;
  query.info = info;
  return qid;
}

void UGC::setupQuery(QHandle handle, const QueryInfo& info) {
#define SET_VAR(var, func)                                                                                             \
  if (info.var)                                                                                                        \
    FUNC(SetReturn##func)(m_ptr, handle, true);
  SET_VAR(additional_previews, AdditionalPreviews)
  SET_VAR(children, Children)
  SET_VAR(key_value_tags, KeyValueTags)
  SET_VAR(long_description, LongDescription)
  SET_VAR(metadata, Metadata)
  SET_VAR(only_ids, OnlyIDs)
  SET_VAR(playtime_stats, PlaytimeStats)
  SET_VAR(total_only, TotalOnly)
#undef SET
  if (!info.search_text.empty())
    FUNC(SetSearchText)(m_ptr, handle, info.search_text.c_str());
}

UGC::QueryId UGC::createQuery(const QueryInfo& info, vector<ItemId> items) {
  CHECK(items.size() >= 1);

  auto handle = FUNC(CreateQueryUGCDetailsRequest)(m_ptr, items.data(), items.size());
  CHECK(handle != invalidHandle);
  // TODO: properly handle errors

  setupQuery(handle, info);
  auto qid = allocQuery(handle, info);
  m_queries[qid].items = std::move(items);
  m_queries[qid].call = FUNC(SendQueryUGCRequest)(m_ptr, handle);
  return qid;
}

UGC::QueryId UGC::createQuery(const QueryInfo& info, EUGCQuery type, EUGCMatchingUGCType matching_type, unsigned app_id,
                              int page_id) {
  CHECK(page_id >= 1);
  auto handle = FUNC(CreateQueryAllUGCRequest)(m_ptr, type, matching_type, app_id, app_id, page_id);
  CHECK(handle != k_UGCQueryHandleInvalid);
  // TODO: properly handle errors

  setupQuery(handle, info);

  auto qid = allocQuery(handle, info);
  m_queries[qid].call = FUNC(SendQueryUGCRequest)(m_ptr, handle);
  return qid;
}

void UGC::updateQueries(Utils& utils) {
  for (int n = 0; n < m_queries.size(); n++) {
    auto& query = m_queries[n];
    if (!query.valid())
      continue;

    if (query.call.status == QStatus::pending)
      query.call.update(utils);
  }
}

void UGC::finishQuery(QueryId qid) {
  CHECK(isQueryValid(qid));
  FUNC(ReleaseQueryUGCRequest)(m_ptr, m_queries[qid].handle);
  m_queries[qid].handle = invalidHandle;
}

bool UGC::isQueryValid(QueryId qid) const {
  return m_queries[qid].valid();
}

QueryStatus UGC::queryStatus(QueryId qid) const {
  CHECK(isQueryValid(qid));
  return m_queries[qid].call.status;
}

const QueryInfo& UGC::queryInfo(QueryId qid) const {
  CHECK(isQueryValid(qid));
  return m_queries[qid].info;
}

QueryResults UGC::queryResults(QueryId qid) const {
  CHECK(queryStatus(qid) == QStatus::completed);
  auto& result = m_queries[qid].call.result();
  return {(int)result.m_unNumResultsReturned, (int)result.m_unTotalMatchingResults};
}

const char* UGC::queryError(QueryId qid) const {
  CHECK(isQueryValid(qid));
  auto& call = m_queries[qid].call;
  if (call.status == QStatus::failed)
    return call.failText();
  return "";
}

QueryDetails UGC::queryDetails(QueryId qid, int index) {
  CHECK(queryStatus(qid) == QStatus::completed);
  auto& query = m_queries[qid];

  QueryDetails out;
  auto result = FUNC(GetQueryUGCResult)(m_ptr, query.handle, index, &out);
  CHECK(result);
  // TODO: properly handle errors
  return out;
}

string UGC::queryMetadata(QueryId qid, int index) {
  CHECK(queryStatus(qid) == QStatus::completed);
  auto& query = m_queries[qid];
  CHECK(query.info.metadata);

  char buffer[4096];
  auto result = FUNC(GetQueryUGCMetadata)(m_ptr, query.handle, index, buffer, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = 0;
  CHECK(result);
  return buffer;
}

vector<pair<string, string>> UGC::queryKeyValueTags(QueryId qid, int index) {
  vector<pair<string, string>> out;

  CHECK(queryStatus(qid) == QStatus::completed);
  auto& query = m_queries[qid];
  CHECK(query.info.key_value_tags);

  char buf1[4096], buf2[4096];
  auto count = FUNC(GetQueryUGCNumKeyValueTags)(m_ptr, query.handle, index);

  out.resize(count);
  for (unsigned n = 0; n < count; n++) {
    auto result =
        FUNC(GetQueryUGCKeyValueTag)(m_ptr, query.handle, index, n, buf1, sizeof(buf1) - 1, buf2, sizeof(buf2) - 1);
    CHECK(result);
    buf1[sizeof(buf1) - 1] = 0;
    buf2[sizeof(buf2) - 1] = 0;
    out[n] = make_pair(buf1, buf2);
  }
  return out;
}
}
