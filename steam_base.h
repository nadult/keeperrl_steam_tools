#pragma once

#include "steam/steamclientpublic.h"
#include "keeperrl/util.h"

// Notes:
//
// - Whole interface is NOT thread safe, it should be used on a single thread only
//
// - Most big classes match steamworks interfaces (Client, Friends, etc.) but some
//   interfaces (like Utils) are merged into Client class (for convenience).
//
// - Client interface is a singleton and holds all the other interfaces within itself
//   user only has to create Client to get access to all the other interfaces.

// TODO: namespace
RICH_ENUM(QueryStatus, invalid, pending, completed, failed);

namespace steam {

class Client;
class Friends;
class UGC;
class Utils;
class User;

bool initAPI();
void runCallbacks();

template <class T> class CallResult;
}
