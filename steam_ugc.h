#pragma once

#include "steam_base.h"
#include <steam/isteamugc.h>

namespace steam {
class UGC {
  public:
  using FileID = PublishedFileId_t;
  int numSubscribedItems() const;

  // TODO: consitency with get suffix
  vector<FileID> subscribedItems() const;

  private:
  UGC(intptr_t);
  friend class Client;

  intptr_t m_ptr;
};
}
