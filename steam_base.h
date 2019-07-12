#pragma once

#include "steam/steamclientpublic.h"
#include "keeperrl/util.h"

#ifndef CHECK
#include <cassert>
#define CHECK assert
#endif

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
