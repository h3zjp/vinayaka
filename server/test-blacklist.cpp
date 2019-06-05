#include <iostream>

#include "distsn.h"


using namespace std;


int main ()
{
	Blacklist blacklist;
	auto users_and_speeds_raw = get_users_and_speed_impl (0.2 / (24.0 * 60.0 * 60.0));
	for (auto i: users_and_speeds_raw) {
		string host_name = i.host;
		string user_name = i.username;
		if (blacklist (host_name, user_name)) {
			cout << host_name << " " << user_name << endl;
		}
	}
}

