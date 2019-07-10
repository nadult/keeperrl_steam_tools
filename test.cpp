#include <stdio.h>

#include <steam/steam_api_common.h>
#include <steam/steam_api.h>
#include <steam/steam_api_flat.h>

#include <string>
#include <vector>
#include <cassert>	

using namespace std;

namespace steam {

class Friends {
public:
#define CALL(name, ...) SteamAPI_ISteamFriends_##name(m_ptr __VA_OPT__(,) __VA_ARGS__)
	Friends(ISteamFriends *ptr) :m_ptr(intptr_t(ptr)) {
		assert(m_ptr);
	}
	
	CSteamID getFriendByIndex(int idx, unsigned flags = k_EFriendFlagAll) const {
		return CSteamID(CALL(GetFriendByIndex, idx, flags));
	}

	string getFriendName(CSteamID friend_id) const {
		return CALL(GetFriendPersonaName, friend_id);
	}
 
	int numFriends(unsigned flags = k_EFriendFlagAll) const {
		return CALL(GetFriendCount, flags);
	}

	private:
#undef CALL
	intptr_t m_ptr;
};


class Client {
#define CALL(name, ...) SteamAPI_ISteamClient_##name(m_ptr __VA_OPT__(,) __VA_ARGS__)
	public:
	Client() {
		// TODO: handle errors, use Expected<>
		m_ptr = (intptr_t)::SteamClient();
		m_pipe = CALL(CreateSteamPipe);
		m_user = CALL(ConnectToGlobalUser, m_pipe);
	}
	~Client() {
		CALL(ReleaseUser, m_pipe, m_user);
		CALL(BReleaseSteamPipe, m_pipe);
	}
	
	Client(const Client&) = delete;
	void operator=(const Client&) = delete;
	
	Friends getFriends() const {
		return CALL(GetISteamFriends, m_user, m_pipe, STEAMFRIENDS_INTERFACE_VERSION);
	}
	
	
	private:
	#undef CALL
	static constexpr const char *version = "144";
	intptr_t m_ptr;
	HSteamPipe m_pipe;
	HSteamUser m_user;
	ISteamFriends *m_friends;
};

}

void printFriends(const steam::Friends &friends) {
	int count = friends.numFriends();
	for(int n = 0; n < count; n++) {
		auto id = friends.getFriendByIndex(n);
		printf("Friend #%d: %s [%ull]\n", n, friends.getFriendName(id).c_str(), id);
	}
}

int main() {
	if (!SteamAPI_Init()) {
		printf("Steam is not running\n");
		return 0;
	}
	
	steam::Client client;
	auto friends = client.getFriends();

	printFriends(friends);
	
	return 0;
}