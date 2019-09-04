#include "distsn.h"


using namespace std;


Optout::Optout ()
{
	map <User, Profile> profiles = read_profiles ();
	for (auto i: profiles) {
		User user = i.first;
		Profile profile = i.second;

		string bio = profile.bio;
		if (users_to_bio.find (user) == users_to_bio.end ()) {
			users_to_bio.insert (pair <User, string> {user, bio});
		}

		bool implicitly_discoverable = profile.implicitly_discoverable;
		if (users_to_implicitly_discoverable.find (user) == users_to_implicitly_discoverable.end ()) {
			users_to_implicitly_discoverable.insert (pair <User, bool> {user, implicitly_discoverable});
		}
	}
}


bool Optout::operator () (string a_host_name, string a_user_name) const
{
	User user {a_host_name, a_user_name};

	string bio;
	if (users_to_bio.find (user) != users_to_bio.end ()) {
		bio = users_to_bio.at (user);
	}

	bool implicitly_discoverable = true;
	if (users_to_implicitly_discoverable.find (user) != users_to_implicitly_discoverable.end ()) {
		implicitly_discoverable = users_to_implicitly_discoverable.at (user);
	}

	return (* this) (bio, implicitly_discoverable);
}


static string to_lower_case (string in)
{
	string out;
	for (auto c: in) {
		out.push_back ('A' <= c && c <= 'Z'? c - 'A' + 'a': c);
	}
	return out;
}


static string mastodon_hashtag_in_bio (string x) {
	return
		string {"#\u003cspan\u003e"}
		+ x
		+ string {"\u003c/span\u003e"};
}


bool Optout::operator () (string bio, bool implicitly_discoverable) const
{
	if (! implicitly_discoverable) {
		return true;
	}

	set <string> optout_codes {
		string {"㊙️"},
		string {"#rejectsearchengine"},
		mastodon_hashtag_in_bio (string {"rejectsearchengine"}),
		string {"#rejectvinayaka"},
		mastodon_hashtag_in_bio (string {"rejectvinayaka"}),
		string {"#nobot"},
		mastodon_hashtag_in_bio (string {"nobot"}),
		string {"#nobots"},
		mastodon_hashtag_in_bio (string {"nobots"}),
	};

	bool optouted = any_of (
		optout_codes.begin (),
		optout_codes.end (),
		[bio] (string code)
	{
		string i_bio = to_lower_case (bio);
		string i_code = to_lower_case (code);
		return i_bio.find (i_code) != i_bio.npos;
	});

	return optouted;
}


