#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
#include <ctime>
#include <random>
#include <algorithm>

#include <socialnet-1.h>
#include <languagemodel-1.h>

#include "picojson.h"
#include "distsn.h"


using namespace std;


const unsigned int minimum_popularity = 64;


class UserAndSimilarity {
public:
	string host;
	string user;
	double similarity;
public:
	UserAndSimilarity () { };
	UserAndSimilarity (string a_host, string a_user, double a_similarity):
		host (a_host), user (a_user), similarity (a_similarity) { };
};


static bool by_similarity_desc (const UserAndSimilarity &a, const UserAndSimilarity &b)
{
	return b.similarity < a.similarity;
}


static double get_similarity
	(set <string> listener_set,
	set <string> speaker_set,
	map <string, double> & a_intersection,
	map <string, unsigned int> words_to_popularity)
{
	set <string> intersection;
	set_intersection (listener_set.begin (), listener_set.end (),
		speaker_set.begin (), speaker_set.end (),
		inserter (intersection, intersection.begin ()));

	double similarity = 0;
	a_intersection.clear ();

	for (string word: intersection) {
		unsigned int popularity = minimum_popularity;
		if (words_to_popularity.find (word) != words_to_popularity.end ()) {
			popularity = words_to_popularity.at (word);
		}
		double rarity = static_cast <double> (minimum_popularity) * 10.0 * get_rarity (popularity);
		similarity += rarity;
		a_intersection.insert (pair <string, double> {word, rarity});
	}

	return similarity;
}


class ModelTopology {
public:
	unsigned int word_length;
	unsigned int vocabulary_size;
public:
	ModelTopology () { };
	ModelTopology (unsigned int a_word_length, unsigned int a_vocabulary_size) {
		word_length = a_word_length;
		vocabulary_size = a_vocabulary_size;
	};
public:
	bool operator < (const ModelTopology &right) const {
		return vocabulary_size < right.vocabulary_size
			|| (vocabulary_size == right.vocabulary_size && word_length < right.word_length);
	}
};


static set <string> get_words_of_listener
	(vector <string> toots,
	vector <ModelTopology> models,
	map <string, unsigned int> words_to_popularity)
{
	set <string> words;
	for (auto model: models) {
		vector <string> words_in_a_model
			= get_words_from_toots (toots, model.word_length, model.vocabulary_size, words_to_popularity, minimum_popularity);
		words.insert (words_in_a_model.begin (), words_in_a_model.end ());
	}
	return words;
};


static map <User, set <string>> get_words_of_speakers (string filename)
{
	FileLock lock {filename, LOCK_SH};

	map <User, set <string>> users_to_words;
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		cerr << "File not found: " << filename << endl;
	} else {
		try {
			vector <vector <string>> table = parse_csv (in);
			fclose (in);
			set <User> users_set;
			for (auto row: table) {
				if (2 < row.size ()) {
					users_set.insert (User {row.at (0), row.at (1)});
				}
			}
			vector <User> users_vector {users_set.begin (), users_set.end ()};
			for (unsigned int cn = 0; cn < users_vector.size (); cn ++) {
				users_to_words.insert (pair <User, set <string>> {users_vector.at (cn), set <string> {}});
			}
			for (auto row: table) {
				if (2 < row.size ()) {
					User user {row.at (0), row.at (1)};
					string word {row.at (2)};
					if (users_to_words.find (user) != users_to_words.end ()) {
						users_to_words.at (user).insert (word);
					}
				}
			}
		} catch (ParseException e) {
			cerr << "ParseException " << e.line << " " << filename << endl;
		}
	}
	return users_to_words;
}


static map <string, unsigned int> get_words_to_popularity (string filename)
{
	FileLock lock {filename, LOCK_SH};

	map <string, unsigned int> words_to_popularity;
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		cerr << "File not found: " << filename << endl;
	} else {
		try {
			vector <vector <string>> table = parse_csv (in);
			fclose (in);
			for (auto row: table) {
				if (1 < row.size ()) {
					string word = row.at (0);
					stringstream popularity_stream {row.at (1)};
					unsigned int popularity;
					popularity_stream >> popularity;
					if (minimum_popularity < popularity
						&& words_to_popularity.find (word) == words_to_popularity.end ())
					{
						words_to_popularity.insert (pair <string, unsigned int> {word, popularity});
					}
				}
			}
		} catch (ParseException e) {
			cerr << "ParseException " << e.line << " " << filename << endl;
		}
	}
	return words_to_popularity;
}


class WordAndRarity {
public:
	string word;
	double rarity;
public:
	WordAndRarity () { };
	WordAndRarity (string a_word, double a_rarity): word (a_word), rarity (a_rarity) { };
};


bool by_rarity_desc (const WordAndRarity &a, const WordAndRarity &b)
{
	return b.rarity < a.rarity;
};


static string format_result
	(vector <UserAndSimilarity> speakers_and_similarity,
	map <User, map <string, double>> speaker_to_intersection,
	map <User, Profile> users_to_profile,
	set <socialnet::HostNameAndUserName> friends)
{
	stringstream out;
	out << "[";
	for (unsigned int cn = 0; cn < speakers_and_similarity.size () && cn < 400; cn ++) {
		if (0 < cn) {
			out << ",";
		}
		auto speaker = speakers_and_similarity.at (cn);

		map <string, double> intersection_map;
		if (speaker_to_intersection.find (User {speaker.host, speaker.user}) != speaker_to_intersection.end ()) {
			intersection_map = speaker_to_intersection.at (User {speaker.host, speaker.user});
		}
		vector <WordAndRarity> intersection_vector;
		for (auto word_to_rarity: intersection_map) {
			intersection_vector.push_back (WordAndRarity {word_to_rarity.first, word_to_rarity.second});
		}
		stable_sort (intersection_vector.begin (), intersection_vector.end (), by_rarity_desc);

		out
			<< "{"
			<< "\"host\":\"" << escape_json (speaker.host) << "\","
			<< "\"user\":\"" << escape_json (speaker.user) << "\","
			<< "\"similarity\":" << scientific << speaker.similarity << ",";

		string activitypub_id = speaker.user;

		if (users_to_profile.find (User {speaker.host, speaker.user}) == users_to_profile.end ()) {
			out
				<< "\"screen_name\":\"\","
				<< "\"bio\":\"\","
				<< "\"avatar\":\"\","
				<< "\"type\":\"\","
				<< "\"url\":\"\","
				<< "\"implementation\":\"unknown\",";
		} else {
			Profile profile = users_to_profile.at (User {speaker.host, speaker.user});
			activitypub_id = profile.activitypub_id;
			out
				<< "\"screen_name\":\"" << escape_json (profile.screen_name) << "\","
				<< "\"bio\":\"" << escape_json (profile.bio) << "\",";
			if (safe_url (profile.avatar)) {
				out << "\"avatar\":\"" << escape_json (profile.avatar) << "\",";
			} else {
				out << "\"avatar\":\"\",";
			}
			out << "\"type\":\"" << escape_json (profile.type) << "\",";
			out << "\"url\":\"" << escape_json (profile.url) << "\",";
			out << "\"implementation\":\"" << socialnet::format (profile.implementation) << "\",";
		}

		bool following_bool = socialnet::following (speaker.host, speaker.user, activitypub_id, friends);
		out << "\"following\":" << (following_bool? "true": "false") << ",";

		out << "\"intersection\":[";
		for (unsigned int cn_intersection = 0; cn_intersection < intersection_vector.size (); cn_intersection ++) {
			if (0 < cn_intersection) {
				out << ",";
			}
			string word = intersection_vector.at (cn_intersection).word;
			double rarity = intersection_vector.at (cn_intersection).rarity;
			out << "{";
			out << "\"word\":\"" << escape_json (escape_utf8_fragment (word)) << "\",";
			out << "\"rarity\":" << scientific << rarity;
			out << "}";
		}
		out << "]";

		out << "}";
	}
	out << "]";
	return out.str ();
}


static string to_lower (string in)
{
	string out;
	for (auto c: in) {
		if ('A' <= c && c <= 'Z') {
			out.push_back (c - 'A' + 'a');
		} else {
			out.push_back (c);
		}
	}
	return out;
}


static double get_affirmative_action_rate (string bio)
{
	set <string> words {
		string {"♀"},
		string {"⚦"},
		string {"⚧"},
		string {"⚪"},
		string {"⚨"},
		string {"⚩"},
		string {"⚲"},
		string {"female"},
		string {"she/her"},
		string {"transgender"},
		string {"mtf"},
		string {"ftm"},
		string {"agender"},
		string {"queer"},
		string {"questioning"},
	};

	bool hit = any_of (words.begin (), words.end (), [bio] (auto x) {
		return to_lower (bio).find (x) != string::npos;
	});

	return hit? 1.5: 1.0;
}


int main (int argc, char **argv)
{
	if (argc < 3) {
		exit (1);
	}
	string host {argv [1]};
	string user {argv [2]};

	cerr << user << "@" << host << endl;

	cerr << "get_optouted_users" << endl;
	set <User> optouted_users = get_optouted_users ();
	if (optouted_users.find (User {host, user}) != optouted_users.end ()) {
		cerr << "optouted." << endl;

		vector <UserAndSimilarity> dummy_speakers_and_similarity;
		for (unsigned int cn = 0; cn < 3; cn ++) {
			dummy_speakers_and_similarity.push_back
				(UserAndSimilarity {"3.distsn.org", "optout", 0.0});
		}

		map <User, map <string, double>> dummy_speaker_to_intersection;
		set <socialnet::HostNameAndUserName> dummy_friends;

		Profile dummy_profile;
		dummy_profile.screen_name = string {"Vinayaka Optouting Info"};
		dummy_profile.type = string {"Person"};
		dummy_profile.url = string {"https://3.distsn.org/optout"};
		dummy_profile.implementation = socialnet::eImplementation::PLEROMA;
		dummy_profile.number_of_followers = 0;

		map <User, Profile> dummy_users_to_profile {
			pair <User, Profile> {
				User {"3.distsn.org", "optout"},
				dummy_profile
			}
		};

		string result = format_result
			(dummy_speakers_and_similarity,
			dummy_speaker_to_intersection,
			dummy_users_to_profile,
			dummy_friends);
	
		add_to_cache (host, user, result);
		return 0;
	}

	auto http = make_shared <socialnet::Http> ();
	http->user_agent = user_agent;
	auto socialnet_user = socialnet::make_user (host, user, http);

	string screen_name;
	string bio;
	string avatar;
	string type;
	cerr << "get_profile" << endl;
	socialnet_user->get_profile (screen_name, bio, avatar, type);

	vector <string> toots;
	if (! screen_name.empty ()) {
		toots.push_back (screen_name);
	}
	if (! bio.empty ()) {
		toots.push_back (bio);
	}

	vector <socialnet::Status> socialnet_statuses = socialnet_user->get_timeline (10);

	for (unsigned int cn = 0; cn < socialnet_statuses.size () && cn < 80; cn ++) {
		toots.push_back (socialnet_statuses.at (cn).content);
	}
	
	vector <UserAndSimilarity> speakers_and_similarity;
	map <User, map <string, double>> speaker_to_intersection;
	map <User, Profile> users_to_profile;

	if (4 <= toots.size ()) {
		/* Sorry to this long long scope. */
	
		const unsigned int vocabulary_size {3200};
		vector <ModelTopology> models = {
			ModelTopology {6, vocabulary_size},
		};
	
		cerr << "get_words_to_popularity" << endl;
		map <string, unsigned int> words_to_popularity
			= get_words_to_popularity (string {"/var/lib/vinayaka/model/popularity.csv"});
		
		cerr << "get_words_of_listener" << endl;
		set <string> words_of_listener = get_words_of_listener (toots, models, words_to_popularity);
	
		cerr << "get_words_of_speakers" << endl;
		map <User, set <string>> speaker_to_words
			= get_words_of_speakers (string {"/var/lib/vinayaka/model/concrete-user-words.csv"});

		cerr << "read_profiles" << endl;
		users_to_profile = read_profiles ();

		cerr << "detect listener's implementation" << endl;
		auto listener_implementation = socialnet::get_implementation (host, * http);
		if (is_misskey (listener_implementation)) {
			map <User, set <string>> speaker_to_words_with_good_implementations;
			for (auto i: speaker_to_words) {
				User user = i.first;
				set <string> words = i.second;
				socialnet::eImplementation speaker_implementation
					= socialnet::eImplementation::UNKNOWN;
				if (users_to_profile.find (user) != users_to_profile.end ()) {
					Profile profile = users_to_profile.at (user);
					speaker_implementation = profile.implementation;
				}
				if (! is_misskey (speaker_implementation)) {
					speaker_to_words_with_good_implementations.insert
						(pair <User, set <string>> {user, words});
				}
			}
			speaker_to_words = speaker_to_words_with_good_implementations;
		}
	
		cerr << "get_similarity" << endl;
		unsigned int cn = 0;
		for (auto speaker_and_words: speaker_to_words) {
			if (cn % 100 == 0) {
				cerr << cn << " ";
			}
			cn ++;

			User speaker = speaker_and_words.first;
			set <string> words_of_speaker = speaker_and_words.second;
			map <string, double> intersection;
			double similarity = get_similarity (words_of_listener, words_of_speaker, intersection, words_to_popularity);
			UserAndSimilarity speaker_and_similarity;
			speaker_and_similarity.user = speaker.user;
			speaker_and_similarity.host = speaker.host;
			speaker_and_similarity.similarity = similarity;
			speakers_and_similarity.push_back (speaker_and_similarity);
			speaker_to_intersection.insert (pair <User, map <string, double>> {speaker, intersection});
		}
		cerr << endl;

		cerr << "affirmative_action" << endl;
		for (auto &speaker_and_similarity: speakers_and_similarity) {
			string host_name = speaker_and_similarity.host;
			string user_name = speaker_and_similarity.user;
			User user {host_name, user_name};
			if (users_to_profile.find (user) != users_to_profile.end ()) {
				auto profile = users_to_profile.at (user);
				double affirmative_action_rate = get_affirmative_action_rate (profile.bio);
				speaker_and_similarity.similarity *= affirmative_action_rate;
			}
		}		

		cerr << "stable_sort" << endl;
		stable_sort (speakers_and_similarity.begin (), speakers_and_similarity.end (), by_similarity_desc);
	} else {
		/* toots.size () < 4 */
		
		string s;
		{
			string file_name {"/var/lib/vinayaka/users-new-cache.json"};
			FileLock lock {file_name, LOCK_SH};
			FILE *in = fopen (file_name.c_str (), "r");
			if (in == nullptr) {
				cerr << file_name << " can not open." << endl;
				exit (1);
			}
			for (; ; ) {
			char b [1024];
				auto fgets_return = fgets (b, 1024, in);
				if (fgets_return == nullptr) {
					break;
				}
				s += string {b};
			}
		}
		picojson::value json_value;
		string json_parse_error = picojson::parse (json_value, s);
		if (! json_parse_error.empty ()) {
			cerr << json_parse_error << endl;
			exit (1);
		}

		auto users_array = json_value.get <picojson::array> ();

		for (unsigned int cn = 0; cn < users_array.size (); cn ++) {
			auto user_value = users_array.at (cn);
			auto user_object = user_value.get <picojson::object> ();
			string host = user_object.at (string {"host"}).get <string> ();
			string user = user_object.at (string {"user"}).get <string> ();

			string screen_name = user_object.at (string {"screen_name"}).get <string> ();
			string bio = user_object.at (string {"bio"}).get <string> ();
			string avatar = user_object.at (string {"avatar"}).get <string> ();

			string type = user_object.at (string {"type"}).get <string> ();

			bool described_bool = described (screen_name, bio, avatar);

			if (described_bool) {
				UserAndSimilarity speaker_and_similarity;
				speaker_and_similarity.host = host;
				speaker_and_similarity.user = user;
				speaker_and_similarity.similarity = static_cast <double> (0);
				speakers_and_similarity.push_back (speaker_and_similarity);

				User speaker {host, user};
				map <string, double> intersection;
				speaker_to_intersection.insert (pair <User, map <string, double>> {speaker, intersection});
				
				Profile profile;
				profile.screen_name = screen_name;
				profile.bio = bio;
				profile.avatar = avatar;
				profile.type = type;
				profile.url = string {};
				profile.implementation = socialnet::eImplementation::UNKNOWN;
				users_to_profile.insert (pair <User, Profile> {speaker, profile});
			}
		}
	}

	cerr << "get_friends_no_exception" << endl;
	set <socialnet::HostNameAndUserName> friends = socialnet_user->get_friends_no_exception ();

	cerr << "format_result" << endl;
	string result = format_result
		(speakers_and_similarity,
		speaker_to_intersection,
		users_to_profile,
		friends);
	
	cerr << "add_to_cache" << endl;
	add_to_cache (host, user, result);
}



