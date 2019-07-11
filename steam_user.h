#pragma once

#include "steam_base.h"
#include <steam/isteamuser.h>

namespace steam {
class User {
  public:
  CSteamID id() const;

  private:
  User(intptr_t);
  friend class Client;

  intptr_t m_ptr;
};
}
