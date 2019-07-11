#pragma once

#include "steam_base.h"
#include <steam/isteamugc.h>

namespace steam {

using ItemId = PublishedFileId_t;

struct DownloadInfo {
  unsigned long long bytes_downloaded;
  unsigned long long bytes_total;
};

struct InstallInfo {
  unsigned long long size_on_disk;
  string folder;
  unsigned time_stamp;
};

struct QueryInfo {
  string search_text;
  unsigned playtime_stats_days = 0;
  bool long_description = false;
  bool key_value_tags = false;
  bool metadata = false;
  bool only_ids = false;
  bool children = false;
  bool additional_previews = false;
  bool total_only = false;
};

class UGCQuery {
  public:
  using Handle = UGCQueryHandle_t;
  using Info = QueryInfo;

  ~UGCQuery();

  bool isCompleted() const {
    return m_is_completed;
  }
  bool isValid() const {
    return m_handle != k_UGCQueryHandleInvalid;
  }

  int numResults() const {
    return m_num_results;
  }
  int totalResults() const {
    return m_total_results;
  }

  string metadata(int result_id) const;

  private:
  UGCQuery(intptr_t);
  friend class UGC;

  intptr_t m_ugc;
  vector<ItemId> m_items;
  QueryInfo m_info;
  Handle m_handle = k_UGCQueryHandleInvalid;
  bool m_is_completed = false;

  int m_num_results = 0, m_total_results = 0;
};

class UGC {
  public:
  using Query = UGCQuery;
  using QueryId = unsigned;

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

  QueryId createQuery(const QueryInfo&, vector<ItemId>);
  bool isCompleted(QueryId) const;
  Query& readQuery(QueryId);
  void finishQuery(QueryId);

  private:
  UGC(intptr_t);
  friend class Client;

  // TODO: is it safe on windows?
  STEAM_CALLBACK_MANUAL(UGC, onQueryCompleted, SteamUGCQueryCompleted_t, m_query_completed);

  vector<UGCQuery> m_queries;

  intptr_t m_ptr;
};
}
