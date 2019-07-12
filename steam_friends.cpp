#include "steam_internal.h"
#include "steam_friends.h"

#define FUNC(name, ...) SteamAPI_ISteamFriends_##name

namespace steam {
Friends::Friends(intptr_t ptr) : m_ptr(ptr) {
}

int Friends::count(unsigned flags) const {
  return FUNC(GetFriendCount)(m_ptr, flags);
}

vector<CSteamID> Friends::ids(unsigned flags) const {
  int count = this->count(flags);
  vector<CSteamID> out;
  out.reserve(count);
  for (int n = 0; n < count; n++)
    out.emplace_back(CSteamID(FUNC(GetFriendByIndex)(m_ptr, n, flags)));
  return out;
}

string Friends::name(CSteamID friend_id) const {
  return FUNC(GetFriendPersonaName)(m_ptr, friend_id);
}

int Friends::avatar(CSteamID friend_id, int size) const {
  CHECK(size >= 0 && size <= 2);
  if (size == 0)
    return FUNC(GetSmallFriendAvatar)(m_ptr, friend_id);
  else if (size == 1)
    return FUNC(GetMediumFriendAvatar)(m_ptr, friend_id);
  else // size == 2
    return FUNC(GetLargeFriendAvatar)(m_ptr, friend_id);
}
}
