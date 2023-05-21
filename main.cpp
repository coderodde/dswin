#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <ShlObj.h>
#include <vector>

using std::cerr;
using std::cout;
using std::find_if;
using std::getline;
using std::ifstream;
using std::ios_base;
using std::isspace;
using std::istream;
using std::malloc;
using std::min;
using std::ofstream;
using std::ostream;
using std::size_t;
using std::strcpy;
using std::strlen;
using std::string;
using std::vector;

static const string TAG_FILE_NAME = "tags";
static const string COMMAND_FILE_NAME = "ds_command.cmd";
static const string PREVIOUS_DIRECTORY_TAG_NAME = "previous_directory";

static inline void ltrim(std::string& s) {
	s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !isspace(ch);
		}));
}

static inline void rtrim(std::string& s) {
	s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !isspace(ch);
		}).base(), s.end());
}

static inline void trim(std::string& s) {
	rtrim(s);
	ltrim(s);
}

static char* get_home_directory_name() {
	CHAR path[MAX_PATH];
	// Load the home path name:
	SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
	// Get the length of the home path name:
	size_t len = strlen(path);
	// Allocate space for the home path name copy:
	char* copy_path = new char[len + 1];
	// Copy the home path name:
	strcpy(copy_path, path);
	// Zero-terminate the home path name copy:
	copy_path[len] = '\0';

	return copy_path;
}

static ifstream get_tag_file_ifstream(string const& tag_file_path) {
	ifstream ifs(tag_file_path);
	return ifs;
}

static ofstream get_tag_file_ofstream(string const& tag_file_path) {
	ofstream ofs(tag_file_path, ios_base::trunc);
	return ofs;
}

static ofstream get_command_file_stream(string const& command_file_path) {
	ofstream ofs(command_file_path, ios_base::trunc);
	return ofs;
}

static size_t levenshtein_distance(string const& str1, string const& str2) {
	vector<vector<size_t>> matrix;

	// Initialize the matrix setting all entries to zero:
	for (size_t row_index = 0; row_index <= str1.length(); row_index++) {
		vector<size_t> row_vector;
		row_vector.resize(str2.length() + 1);
		matrix.push_back(row_vector);
	}

	// Initialize the leftmost column:
	for (size_t row_index = 1; row_index <= str1.length(); row_index++) {
		matrix.at(row_index).at(0) = row_index;
	}

	// Initialize the topmost row:
	for (size_t column_index = 1;
		column_index <= str2.length();
		column_index++) {
		matrix.at(0).at(column_index) = column_index;
	}

	for (size_t column_index = 1;
		column_index <= str2.length();
		column_index++) {

		for (size_t row_index = 1; row_index <= str1.length(); row_index++) {
			size_t substitution_cost;

			if (str1.at(row_index - 1) == str2.at(column_index - 1)) {
				substitution_cost = 0;
			}
			else {
				substitution_cost = 1;
			}

			matrix.at(row_index).at(column_index) =
				min(min(matrix.at(row_index - 1).at(column_index) + 1,
					matrix.at(row_index).at(column_index - 1) + 1),
					matrix.at(row_index - 1)
					.at(column_index - 1) + substitution_cost);
		}
	}

	return matrix.at(str1.length()).at(str2.length());
}

static string get_current_directory_name() {
	char path_name[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH - 1, path_name);
	return string(path_name);
}

class DirectoryTagEntry {
private:
	string tag;
	string dir;

public:
	DirectoryTagEntry(string tag_, string dir_) :
		tag{ tag_ },
		dir{ dir_ }
	{

	}

	string get_tag() const {
		return tag;
	}

	string get_dir() const {
		return dir;
	}

	void set_dir(string const& new_dir) {
		dir = new_dir;
	}
};

std::ostream& operator << (ostream& os, DirectoryTagEntry const& dte) {
	os << dte.get_tag() << " " << dte.get_dir();
	return os;
}

std::istream& operator >> (istream& is, vector<DirectoryTagEntry>& entries) {
	string tag;
	string dir;
	is >> tag;
	getline(is, dir);
	trim(tag);
	trim(dir);
	DirectoryTagEntry dte(tag, dir);
	entries.push_back(dte);
	return is;
}

static string get_tag_file_path() {
	return get_home_directory_name() + string("\\") + TAG_FILE_NAME;
}

static string get_cmd_file_path() {
	return get_home_directory_name() + string("\\") + COMMAND_FILE_NAME;
}

static bool tag_matches_previous_directory_tag(DirectoryTagEntry& entry) {
	return entry.get_tag() == PREVIOUS_DIRECTORY_TAG_NAME;
}

static vector<DirectoryTagEntry>::iterator
find_previous_directory_tag_entry_iterator(
	vector<DirectoryTagEntry>& entries) {
	return find_if(entries.begin(),
		entries.end(),
		[](DirectoryTagEntry& entry) {
			return tag_matches_previous_directory_tag(entry);
		});
}

static void process_previous_no_tag_entry(vector<DirectoryTagEntry>& entries) {
	string current_path = get_current_directory_name();
	DirectoryTagEntry dte(PREVIOUS_DIRECTORY_TAG_NAME, current_path);
	entries.push_back(dte);

	ofstream tag_file_ofs = get_tag_file_ofstream(get_tag_file_path());
	ofstream cmd_file_ofs = get_command_file_stream(get_cmd_file_path());
	size_t index = 0;

	for (DirectoryTagEntry const& dte : entries) {
		tag_file_ofs << dte.get_tag() << " " << dte.get_dir();

		if (index < entries.size() - 1) {
			tag_file_ofs << "\n";
		}

		index++;
	}

	DirectoryTagEntry current_directory_tag_entry(PREVIOUS_DIRECTORY_TAG_NAME,
		get_current_directory_name());

	tag_file_ofs << current_directory_tag_entry;

	cmd_file_ofs << "echo \"Set previous directory to ^\""
		<< current_path
		<< "^\".";

	tag_file_ofs.close();
	cmd_file_ofs.close();
}

static void save_tag_file(vector<DirectoryTagEntry> const& entries,
						  ofstream& ofs) {
	size_t index = 0;

	for (DirectoryTagEntry const& entry : entries) {
		ofs << entry.get_tag() << " " << entry.get_dir();

		if (index < entries.size() - 1) {
			ofs << "\n";
		}

		index++;
	}

	ofs.close();
}

static void process_switch_to_previous(
	vector<DirectoryTagEntry>& entries,
	vector<DirectoryTagEntry>::iterator
	previous_directory_tag_entry_const_iterator) {

	string next_directory_path =
		(*previous_directory_tag_entry_const_iterator).get_dir();

	ofstream cmd_file_ofs = get_command_file_stream(get_cmd_file_path());

	cmd_file_ofs << "cd " << next_directory_path;
	cmd_file_ofs.close();

	string current_directory_path = get_current_directory_name();

	(*previous_directory_tag_entry_const_iterator)
		.set_dir(current_directory_path);

	ofstream tag_file_ofs = get_tag_file_ofstream(get_tag_file_path());

	save_tag_file(entries, tag_file_ofs);
	tag_file_ofs.close();
}

static void process_previous() {
	vector<DirectoryTagEntry> entries;
	string tag_file_path = get_tag_file_path();
	ifstream ifs = get_tag_file_ifstream(tag_file_path);

	while (!ifs.eof() && ifs.good()) {
		ifs >> entries;
	}

	ifs.close();

	vector<DirectoryTagEntry>::iterator
		previous_directory_tag_entry_iterator =
		find_previous_directory_tag_entry_iterator(entries);

	if (previous_directory_tag_entry_iterator == entries.cend()) {
		process_previous_no_tag_entry(entries);
	}
	else {
		process_switch_to_previous(entries, previous_directory_tag_entry_iterator);
	}
}

static void create_previous_tag_entry() {
	string current_directory_path = get_current_directory_name();
	ifstream ifs = get_tag_file_ifstream(get_tag_file_path());
	vector<DirectoryTagEntry> entries;
	bool previous_entry_updated = false;

	while (!ifs.eof() && ifs.good()) {
		string tag;
		string dir;

		ifs >> tag;
		getline(ifs, dir);

		trim(tag);
		trim(dir);

		if (tag == PREVIOUS_DIRECTORY_TAG_NAME) {
			dir = current_directory_path;
			previous_entry_updated = true;
		}

		DirectoryTagEntry dte(tag, dir);
		entries.push_back(dte);
	}

	if (!previous_entry_updated) {
		DirectoryTagEntry dte(PREVIOUS_DIRECTORY_TAG_NAME,
			current_directory_path);

		entries.push_back(dte);
	}

	ifs.close();
	ofstream ofs = get_tag_file_ofstream(get_tag_file_path());
	save_tag_file(entries, ofs);
}

int main(int argc, char* argv[]) {
	if (argc == 1) {
		process_previous();
		return EXIT_SUCCESS;
	}

	if (argc != 2) {
		ofstream ofs = get_command_file_stream(get_cmd_file_path());
		ofs << "@echo off\n";
		ofs << "echo|set /p=\"Expected 1 argument. "
			<< (argc - 1) << " received.\"\n";

		return 1;
	}

	create_previous_tag_entry();

	string target_tag = argv[1];
	string tag_file_name = get_tag_file_path();
	string cmd_file_name = get_cmd_file_path();

	ifstream ifs(tag_file_name);
	ofstream ofs = get_command_file_stream(get_cmd_file_path());
	size_t best_known_levenshtein_distance = SIZE_MAX;
	string best_known_directory;

	if (ifs.eof()) {
		ofs << "echo|set /p=\"The tag file is empty. Please edit ^\""
			<< get_home_directory_name() << "\\tags^\"";

		ofs.close();
		ifs.close();
		return 0;
	}

	while (!ifs.eof() && ifs.good()) {
		string tag;
		string dir;

		ifs >> tag;
		getline(ifs, dir);

		trim(tag);
		trim(dir);

		size_t levenshtein_dist = levenshtein_distance(tag, target_tag);

		if (levenshtein_dist == 0) {
			best_known_directory = dir;
			break;
		}

		if (best_known_levenshtein_distance > levenshtein_dist) {
			best_known_levenshtein_distance = levenshtein_dist;
			best_known_directory = dir;
		}
	}

	ofs << "cd " << best_known_directory;
	ofs.close();
	ifs.close();
	return 0;
}
