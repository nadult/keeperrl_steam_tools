#include "steam_internal.h"
#include "steam_ugc.h"

#define FUNC(name, ...) SteamAPI_ISteamUGC_##name

namespace steam {

UGC::UGC(intptr_t ptr) : m_ptr(ptr) {
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
}
