#include <ctime>
#include <string>

#include "malarga.h"

using namespace Malarga;

const wchar_t* const RecordManager::filename = L"\\malarga.csv";

RecordManager::RecordManager() : records(&compare) {
	EnvProvider::GetFolderForData(path);
	EnvProvider::AppendPathSubitem(path, this->filename);

	load();
}

RecordManager::~RecordManager() { save(); }

bool RecordManager::compare(const record& a, const record& b) {
	return a.mode > b.mode ||
		(a.mode == b.mode &&
			(a.score > b.score ||
				(a.score == b.score &&
					(a.date > b.date || (a.date == b.date && a.time > b.time)))));
}

const wchar_t* const RecordManager::getPath() { return path; }

std::set<RecordManager::record>::const_iterator
RecordManager::recordsIterator() {
	return records.begin();
}

std::set<RecordManager::record>::const_iterator
RecordManager::recordsIteratorEnd() {
	return records.end();
}

void RecordManager::load() {
	std::ifstream is;
	EnvProvider::OpenStream(is, path, std::ios::ate);

	if (!is) return;
	size_t size = (size_t)is.tellg();

	if (size + 1 <= 0) {
		is.close();
		return;
	}

	char* buf = new char[size + 1]{}, * iptr = buf;

	is.seekg(0);
	is.read(buf, size);
	is.close();

	char* cptr;
	while (*iptr != '\0') {
		record r;

		r.date = strtol(iptr, &iptr, 10);
		iptr++;

		if (*iptr == '\"') iptr++;
		char cname[max_name_bytes + 1];

		for (cptr = cname;
			cptr < cname + max_name_bytes && strchr(",\r\n", *iptr) == nullptr;
			cptr++) {
			if (*iptr == '"') {
				iptr++;
				if (*iptr != '"') break;
			}
			*cptr = *iptr;
			iptr++;
		}

		*cptr = '\0';
		EnvProvider::MbsCastWch(cname, r.name,
			PlayerController::max_name_length + 1);
		r.name[PlayerController::max_name_length] = '\0';

		while (strchr(",\"\r\n", *iptr) != nullptr) iptr++;

		r.score = strtol(iptr, &iptr, 10);
		iptr++;
		r.mode = strtol(iptr, &iptr, 10);
		iptr++;
		r.time = strtol(iptr, &iptr, 10);
		iptr++;

		records.emplace(r);
	}

	delete[] buf;
}

void RecordManager::save() {
	std::ofstream ostr;
	EnvProvider::OpenStream(ostr, path);

	char name[max_name_bytes];
	for (record r : records) {
		ostr << r.date << ",\"";
		EnvProvider::WchCastMbs(r.name, name, max_name_bytes);

		for (char* cptr = name; *cptr != '\0'; cptr++) {
			if (*cptr == '"') ostr.put('"');
			ostr.put(*cptr);
		}
		ostr << "\"," << r.score << ',' << r.mode << ',' << r.time << '\n';
	}
	ostr.close();
}

void RecordManager::add(const wchar_t* name, int score, int mode,
	int lastTime) {
	record r;

	time_t t = time(nullptr);
	tm time;
	localtime_s(&time, &t);

	r.date =
		(1900 + time.tm_year) * 10000 + (1 + time.tm_mon) * 100 + time.tm_mday;
	r.time = lastTime;
	wcsncpy(r.name, name, PlayerController::max_name_length);
	r.mode = mode;
	r.score = score;

	records.emplace(r);
}

void RecordManager::remove(const record& r) { records.erase(r); }

int RecordManager::highestScore(const wchar_t* name, int mode) {
	int value = 0;

	for (record r : records) {
		if (wcscmp(name, r.name) == 0 && r.mode == mode && r.score > value)
			value = r.score;
	}

	return value;
}