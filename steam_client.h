#pragma once

#include "steam_base.h"

// TODO: refki pomiędzy klientem a innymi interfejsami?
// TODO: wymyśleć lepszy interfejs

namespace steam {

class Client {
  public:
  Client();
  Client(const Client&) = delete;
  void operator=(const Client&) = delete;
  ~Client();

  static Client& instance();

  Friends& friends();
  User& user();
  Utils& utils();
  UGC& ugc();

  // TODO: mark callback-based functions?
  optional<int> numberOfCurrentPlayers();

  private:
  struct Impl;
  struct Ifaces;
  unique_ptr<Impl> impl;
  unique_ptr<Ifaces> ifaces;
};
}
