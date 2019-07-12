#pragma once

#include "steam_base.h"
#include <steam/isteamutils.h>

namespace steam {

class Utils {
  public:
  // TODO: return expected ?
  pair<int, int> imageSize(int image_id) const;
  vector<uint8> imageData(int image_id) const;

  private:
  Utils(intptr_t);
  friend class Client;

  intptr_t m_ptr;
};
}
