#pragma once

#include "steam_base.h"
#include <steam/isteamclient.h>
#include <memory>

namespace steam {
class Client {
  public:
  Client();
  ~Client();

  Client(const Client&) = delete;
  void operator=(const Client&) = delete;

  Friends friends() const;
  User user() const;
  Utils utils() const;

  UGC& ugc();

  private:
  intptr_t m_ptr;
  HSteamPipe m_pipe;
  HSteamUser m_user;
  std::unique_ptr<UGC> m_ugc;
};
}
