#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>

#ifdef _WIN32
#include <ShlObj.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif
#include <vector>

static const std::string TAG_FILE_NAME = "tags";
static const std::string COMMAND_FILE_NAME = "ds_command";
static const std::string PREVIOUS_DIRECTORY_TAG_NAME = "previous_directory";

static void ltrim(std::string& s) {
	s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !isspace(ch);
		}));
}

static void rtrim(std::string& s) {
	s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !isspace(ch);
		}).base(), s.end());
}

static void trim(std::string& s) {
	rtrim(s);
	ltrim(s);
}

#ifdef _WIN32
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
#else
static char* get_home_directory_name() {
	struct passwd *pw = getpwuid(getuid());
	return pw->pw_dir;
}
#endif

static std::ifstream get_tag_file_ifstream(std::string const& tag_file_path) {
	std::ifstream ifs(tag_file_path);
	return ifs;
}

static std::ofstream get_tag_file_ofstream(std::string const& tag_file_path) {
	std::ofstream ofs(tag_file_path, std::ios_base::trunc);
	return ofs;
}

static std::ofstream get_command_file_stream(std::string const& command_file_path) {
	std::ofstream ofs(command_file_path, std::ios_base::trunc);
	return ofs;
}

static size_t levenshtein_distance(std::string const& str1, std::string const& str2) {
	std::vector<std::vector<size_t>> matrix;

	// Initialize the matrix setting all entries to zero:
	for (size_t row_index = 0; row_index <= str1.length(); row_index++) {
		std::vector<std::size_t> row_vector;
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

			size_t a = matrix.at(row_index - 1).at(column_index) + 1;
			size_t b = matrix.at(row_index).at(column_index - 1) + 1;
			size_t c = matrix.at(row_index - 1).at(column_index - 1) + substitution_cost;
			size_t minab = std::min(a, b);
			matrix.at(row_index).at(column_index) = std::min(minab, c);
			/*
			matrix.at(row_index).at(column_index) =
				std::min(std::min(matrix.at(row_index - 1).at(column_index) + 1,
					matrix.at(row_index).at(column_index - 1) + 1),
					matrix.at(row_index - 1)
					.at(column_index - 1) + substitution_cost);*/
		}
	}

	return matrix.at(str1.length()).at(str2.length());
}

#ifdef _WIN32
static std::string get_current_directory_name() {
	char path_name[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH - 1, path_name);
	return std::string(path_name);
}
#else
static std::string get_current_directory_name() {
	char cwd[4096];
	getcwd(cwd, sizeof(cwd));
	return cwd;
}
#endif

class DirectoryTagEntry {
private:
	std::string tag;
	std::string dir;

public:
	DirectoryTagEntry(std::string tag_, std::string dir_) :
		tag{ tag_ },
		dir{ dir_ }
	{

	}

	std::string get_tag() const {
		return tag;
	}

	std::string get_dir() const {
		return dir;
	}

	void set_dir(std::string const& new_dir) {
		dir = new_dir;
	}
};

std::ostream& operator << (std::ostream& os, DirectoryTagEntry const& dte) {
	os << dte.get_tag() << " " << dte.get_dir();
	return os;
}

std::istream& operator >> (std::istream& is, std::vector<DirectoryTagEntry>& entries) {
	std::string tag;
	std::string dir;
	is >> tag;
	std::getline(is, dir);
	trim(tag);
	trim(dir);
	DirectoryTagEntry dte(tag, dir);
	entries.push_back(dte);
	return is;
}

static std::string get_tag_file_path() {
	return get_home_directory_name() + std::string("\\") + TAG_FILE_NAME;
}

static std::string get_cmd_file_path() {
	return get_home_directory_name() + std::string("\\") + COMMAND_FILE_NAME;
}

static bool tag_matches_previous_directory_tag(DirectoryTagEntry& entry) {
	return entry.get_tag() == PREVIOUS_DIRECTORY_TAG_NAME;
}

static std::vector<DirectoryTagEntry>::iterator
find_previous_directory_tag_entry_iterator(
	std::vector<DirectoryTagEntry>& entries) {
	return find_if(entries.begin(),
		entries.end(),
		[](DirectoryTagEntry& entry) {
			return tag_matches_previous_directory_tag(entry);
		});
}

static void process_previous_no_tag_entry(std::vector<DirectoryTagEntry>& entries) {
	std::string current_path = get_current_directory_name();
	DirectoryTagEntry dte(PREVIOUS_DIRECTORY_TAG_NAME, current_path);
	entries.push_back(dte);

	std::ofstream tag_file_ofs = get_tag_file_ofstream(get_tag_file_path());
	std::ofstream cmd_file_ofs = get_command_file_stream(get_cmd_file_path());
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

static void save_tag_file(std::vector<DirectoryTagEntry> const& entries,
	std::ofstream& ofs) {
	size_t index = 0;

	for (DirectoryTagEntry const& entry : entries) {
		ofs << entry.get_tag() << " " << entry.get_dir();

		if (index < entries.size() - 1) {
			ofs << "\n";
		}

		index++;
	}
}

static void process_switch_to_previous(
	std::vector<DirectoryTagEntry>& entries,
	std::vector<DirectoryTagEntry>::iterator
	previous_directory_tag_entry_const_iterator) {

	std::string next_directory_path =
		(*previous_directory_tag_entry_const_iterator).get_dir();

	std::ofstream cmd_file_ofs = get_command_file_stream(get_cmd_file_path());

	cmd_file_ofs << "cd " << next_directory_path;
	cmd_file_ofs.close();

	std::string current_directory_path = get_current_directory_name();

	(*previous_directory_tag_entry_const_iterator)
		.set_dir(current_directory_path);

	std::ofstream tag_file_ofs = get_tag_file_ofstream(get_tag_file_path());

	save_tag_file(entries, tag_file_ofs);
	tag_file_ofs.close();
}

static void process_previous() {
	std::vector<DirectoryTagEntry> entries;
	std::string tag_file_path = get_tag_file_path();
	std::ifstream ifs = get_tag_file_ifstream(tag_file_path);

	while (!ifs.eof() && ifs.good()) {
		ifs >> entries;
	}

	ifs.close();

	std::vector<DirectoryTagEntry>::iterator
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
	std::string current_directory_path = get_current_directory_name();
	std::ifstream ifs = get_tag_file_ifstream(get_tag_file_path());
	std::vector<DirectoryTagEntry> entries;
	bool previous_entry_updated = false;

	while (!ifs.eof() && ifs.good()) {
		std::string tag;
		std::string dir;

		ifs >> tag;
		std::getline(ifs, dir);

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

	std::ofstream ofs = get_tag_file_ofstream(get_tag_file_path());
	save_tag_file(entries, ofs);
}

int main(int argc, char* argv[]) {
	if (argc == 1) {
		process_previous();
		return EXIT_SUCCESS;
	}

	if (argc != 2) {
		std::ofstream ofs = get_command_file_stream(get_cmd_file_path());
		ofs << "@echo off\n";
		ofs << "echo|set /p=\"Expected 1 argument. "
			<< (argc - 1) << " received.\"\n";

		return 1;
	}

	create_previous_tag_entry();

	std::string target_tag = argv[1];
	std::string tag_file_name = get_tag_file_path();
	std::string cmd_file_name = get_cmd_file_path();

	std::cout << "tag file: " << tag_file_name << "\n";
	std::cout << "cmd file: " << cmd_file_name << "\n";

	std::ifstream ifs(tag_file_name);
	std::ofstream ofs = get_command_file_stream(get_cmd_file_path());
	std::size_t best_known_levenshtein_distance = SIZE_MAX;
	std::string best_known_directory;

	if (ifs.eof()) {
		ofs << "echo|set /p=\"The tag file is empty. Please edit ^\""
			<< get_home_directory_name() << "\\tags^\"";

		ofs.close();
		ifs.close();
		return 0;
	}

	while (!ifs.eof() && ifs.good()) {
		std::string tag;
		std::string dir;

		ifs >> tag;
		std::getline(ifs, dir);

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
