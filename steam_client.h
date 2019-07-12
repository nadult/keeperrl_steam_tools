#pragma once

#include "steam_base.h"
#include <steam/isteamclient.h>
#include <steam/isteamuserstats.h>
#include "steam_utils.h"

// TODO:
// - refki pomiędzy klientem a innymi interfejsami
// - template<> call result ?
// - a moze po prostu zwykle funkcje?
//
// TODO: wymyśleć lepszy interfejs

namespace steam {
class Client {
  public:
  Client();
  ~Client();

  Client(const Client&) = delete;
  void operator=(const Client&) = delete;

  // TODO: keep these as members ?
  Friends friends() const;
  User user() const;
  Utils utils() const;

  UGC& ugc();

  optional<int> numberOfCurrentPlayers();

  private:
  // TODO: move to impl?
  optional<CallResult<NumberOfCurrentPlayers_t>> m_nocp;

  intptr_t m_ptr;
  HSteamPipe m_pipe;
  HSteamUser m_user;
  intptr_t m_user_stats, m_utils;
  unique_ptr<UGC> m_ugc;
};
}
