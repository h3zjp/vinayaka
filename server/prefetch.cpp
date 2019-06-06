#include <iostream>
#include <limits>
#include <unistd.h>
#include <sys/wait.h>

#include <languagemodel-1.h>

#include "distsn.h"


using namespace std;


static vector <User> get_users (unsigned int size)
{
	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	vector <User> users;
	for (unsigned int cn = 0; cn < users_and_speed.size () && cn < size; cn ++) {
		UserAndSpeed user_and_speed = users_and_speed.at (cn);
		User user {user_and_speed.host, user_and_speed.username};
		users.push_back (user);
	}
	return users;
}


int main (int argc, char **argv)
{
	auto all_users = get_users (numeric_limits <unsigned int>::max ());
	
	set <string> all_hosts;
	for (auto user: all_users) {
		all_hosts.insert (user.host);
	}

	auto http = make_shared <socialnet::Http> ();
	set <socialnet::eImplementation> implementations_for_prefetch {
		socialnet::eImplementation::PLEROMA,
		socialnet::eImplementation::SECT,
	};
	set <string> prefetch_hosts;
	for (auto host: all_hosts) {
		cerr << host << endl;
		auto implementation = socialnet::get_implementation (host, * http);
		if (implementations_for_prefetch.find (implementation) != implementations_for_prefetch.end ()) {
			prefetch_hosts.insert (host);
		}
	}

	vector <User> prefetch_users;
	for (auto user: all_users) {
		string host_name = user.host;
		if (prefetch_hosts.find (user.host) != prefetch_hosts.end ()) {
			prefetch_users.push_back (user);
		}
	}

	for (unsigned int cn = 0; cn < 240 && cn < prefetch_users.size (); cn ++) {
		auto user = prefetch_users.at (cn);
		cerr << cn << " " << user.host << " " << user.user << endl;

		auto pid = fork ();
		if (pid < 0) {
			/* Error */
			/* Do nothing */
		} else if (pid == 0) {
			/* Child */
			string path {"/usr/local/bin/vinayaka-user-match-impl"};
			auto result = execl (path.c_str (), path.c_str (), user.host.c_str (), user.user.c_str (), nullptr);
			if (result < 0) {
				cerr << "Can not exec: " << errno << endl;
			}
		} else {
			/* Parent */
			int status = 0;
			wait (& status);
		}
		
		cerr << endl << endl;
	}
}


