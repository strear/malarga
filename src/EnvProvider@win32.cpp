#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "malarga.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace Malarga;

void EnvProvider::WhoAmI(wchar_t* dest, size_t capacity) {
	auto l = (unsigned long)capacity;
	GetUserName(dest, &l);
}

void EnvProvider::GetFolderForData(wchar_t* path) {
	wchar_t* pathRecv = nullptr;

	SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathRecv);

	if (pathRecv != nullptr)
		wcsncpy_s(path, path_len_limit, pathRecv, path_len_limit - 1);

	CoTaskMemFree(pathRecv);
}

void EnvProvider::AppendPathSubitem(
	wchar_t* folder, const wchar_t* subitem) {
	PathAppend(folder, subitem);
}

bool EnvProvider::MbsCastWch(
	const char* source, wchar_t* target, int capacity) {
	return MultiByteToWideChar(CP_UTF8, 0, source, -1, target, capacity);
}

bool EnvProvider::WchCastMbs(
	const wchar_t* source, char* target, int capacity) {
	return WideCharToMultiByte(CP_UTF8, 0, source, -1, target, capacity, 0, nullptr);
}

void EnvProvider::OpenStream(std::ifstream& is,
	const wchar_t* path, std::ios::openmode mode) {
	is.open(path, mode);
}

void EnvProvider::OpenStream(std::ofstream& os,
	const wchar_t* path, std::ios::openmode mode) {
	os.open(path, mode);
}