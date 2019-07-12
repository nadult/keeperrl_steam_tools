#pragma once

#include "steam_base.h"
#include "steam_utils.h"
#include <steam/isteamugc.h>

namespace steam {

using ItemId = PublishedFileId_t;

struct DownloadInfo {
  unsigned long long bytesDownloaded;
  unsigned long long bytesTotal;
};

struct InstallInfo {
  unsigned long long sizeOnDisk;
  string folder;
  unsigned timeStamp;
};

struct QueryInfo {
  string searchText;
  unsigned playtimeStatsDays = 0;
  bool additionalPreviews = false;
  bool children = false;
  bool keyValueTags = false;
  bool longDescription = false;
  bool metadata = false;
  bool onlyIds = false;
  bool playtimeStats = false;
  bool totalOnly = false;
};

struct QueryResults {
  int count, total;
};

using QueryDetails = SteamUGCDetails_t;

class UGC {
  UGC(intptr_t);

  public:
  UGC(const UGC&) = delete;
  void operator=(const UGC&) = delete;
  ~UGC();

  int numSubscribedItems() const;

  vector<ItemId> subscribedItems() const;

  // TODO: return expected everywhere where something may fail ?
  // maybe just return optional?
  uint32_t state(ItemId) const;
  DownloadInfo downloadInfo(ItemId) const;
  InstallInfo installInfo(ItemId) const;

  // --------- Queries --------------------------------------------------------
  // --------------------------------------------------------------------------

  using QueryId = int;

  QueryId createQuery(const QueryInfo&, vector<ItemId>);
  QueryId createQuery(const QueryInfo&, EUGCQuery, EUGCMatchingUGCType, unsigned app_id, int page_id);

  void updateQueries(Utils&);
  void finishQuery(QueryId);

  // TODO: how to report errors?
  bool isQueryValid(QueryId) const;
  QueryStatus queryStatus(QueryId) const;
  const QueryInfo& queryInfo(QueryId) const;
  QueryResults queryResults(QueryId) const;
  const char* queryError(QueryId) const;
  QueryDetails queryDetails(QueryId, int index);
  string queryMetadata(QueryId, int index);
  vector<pair<string, string>> queryKeyValueTags(QueryId, int index);

  // --------- Internal stuff -------------------------------------------------
  // --------------------------------------------------------------------------

  private:
  using QHandle = UGCQueryHandle_t;
  using QStatus = QueryStatus;
  static constexpr QHandle invalidHandle = k_UGCQueryHandleInvalid;

  using QueryCall = CallResult<SteamUGCQueryCompleted_t>;
  struct QueryData {
    bool valid() const {
      return handle != invalidHandle;
    }

    QHandle handle = invalidHandle;
    vector<ItemId> items;
    QueryInfo info;
    QueryCall call;
  };

  QueryId allocQuery(QHandle, const QueryInfo&);
  void setupQuery(QHandle, const QueryInfo&);

  friend class Client;
  vector<QueryData> queries;
  intptr_t ptr;
};
}
