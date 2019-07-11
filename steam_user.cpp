#include "steam_internal.h"
#include "steam_user.h"

#define FUNC(name, ...) SteamAPI_ISteamUser_##name

namespace steam {

User::User(intptr_t ptr) : m_ptr(ptr) {
}

CSteamID User::id() const {
  return FUNC(GetSteamID)(m_ptr);
}
}
