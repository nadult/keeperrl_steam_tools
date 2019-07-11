#include "steam_internal.h"
#include "steam_ugc.h"

#define FUNC(name, ...) SteamAPI_ISteamUGC_##name

namespace steam {

UGC::UGC(intptr_t ptr) : m_ptr(ptr) {
}

int UGC::numSubscribedItems() const {
  return (int)FUNC(GetNumSubscribedItems)(m_ptr);
}

vector<UGC::FileID> UGC::subscribedItems() const {
  vector<FileID> out(numSubscribedItems());
  int result = FUNC(GetSubscribedItems)(m_ptr, out.data(), out.size());
  out.resize(result);
  return out;
}
}
