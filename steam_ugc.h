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

class UGC {
  public:
  int numSubscribedItems() const;

  vector<ItemId> subscribedItems() const;

  // TODO: return expected everywhere where something may fail ?
  // maybe just return optional?
  uint32_t state(ItemId) const;
  DownloadInfo downloadInfo(ItemId) const;
  InstallInfo installInfo(ItemId) const;

  private:
  UGC(intptr_t);
  friend class Client;

  intptr_t m_ptr;
};
}
