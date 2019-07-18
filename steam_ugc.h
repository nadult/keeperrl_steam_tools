#pragma once

#include "steam_base.h"
#include "steam_utils.h"
#include "steam_call_result.h"
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

struct UpdateItemInfo {
  bool valid() const {
    return result == k_EResultOK;
  }

  ItemId itemId;
  EResult result;
  bool requireLegalAgreement;
};

struct ItemInfo {
  optional<string> title;
  optional<string> description;
  optional<string> folder;
  optional<string> preview;
  optional<int> version;
  optional<ERemoteStoragePublishedFileVisibility> visibility;
};

struct QueryResults {
  int count, total;
};

using QueryDetails = SteamUGCDetails_t;

string itemStateText(unsigned);

class UGC {
  STEAM_IFACE_DECL(UGC);

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

  void updateQueries();
  void finishQuery(QueryId);

  // TODO: how to report errors?
  bool isQueryValid(QueryId) const;
  QueryStatus queryStatus(QueryId) const;
  const QueryInfo& queryInfo(QueryId) const;
  QueryResults queryResults(QueryId) const;
  string queryError(QueryId) const;
  QueryDetails queryDetails(QueryId, int index);
  string queryMetadata(QueryId, int index);
  vector<pair<string, string>> queryKeyValueTags(QueryId, int index);

  // TODO: wrap these commands into a simple interface?
  void beginCreateItem();
  optional<UpdateItemInfo> tryCreateItem();
  bool isCreatingItem() const;
  void cancelCreateItem();

  void updateItem(const ItemInfo&, ItemId);
  optional<UpdateItemInfo> tryUpdateItem();
  bool isUpdatingItem();
  void cancelUpdateItem();

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

  vector<QueryData> queries;
  CallResult<CreateItemResult_t> createItemQuery;
  CallResult<SubmitItemUpdateResult_t> updateItemQuery;
};
}
