#include <iostream>
#include <Windows.h>
#include <string>
#include <strsafe.h>

#include "GetWinErrorStr.h"

struct MoveFileItem {
	MoveFileItem* next;
	WCHAR filePath[MAX_PATH];
};

int mp(const WCHAR* toPath, const WCHAR* fromPath, int depth) {
	WCHAR findPath[MAX_PATH];
	StringCchPrintf(findPath, MAX_PATH, L"%s\\*.*", fromPath);
	WIN32_FIND_DATA fi;
	HANDLE fh = FindFirstFile(findPath, &fi);
	if (fh == INVALID_HANDLE_VALUE) {
		std::wcout << L"FindFirstFile(" << depth << L") 에러: " << GetWinErrorStr(GetLastError()) << std::endl;
		return 0;
	}
	int ret = 1;
	MoveFileItem* dirs = NULL;
	MoveFileItem** ppd = &dirs;
	MoveFileItem* files = NULL;
	MoveFileItem** ppf = &files;
	do {
		if (wcscmp(fi.cFileName, L".") == 0 || wcscmp(fi.cFileName, L"..") == 0) continue;
		if (fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			*ppd = new MoveFileItem();
			(*ppd)->next = NULL;
			StringCchPrintf((*ppd)->filePath, MAX_PATH, L"%s\\%s", fromPath, fi.cFileName);
			ppd = &(*ppd)->next;
		}
		else {
			*ppf = new MoveFileItem();
			(*ppf)->next = NULL;
			StringCchCopy((*ppf)->filePath, MAX_PATH, fi.cFileName);
			ppf = &(*ppf)->next;
		}
	} while (FindNextFile(fh, &fi) == TRUE);
	FindClose(fh);
	while (dirs) {
		if (mp(toPath, dirs->filePath, depth + 1)) RemoveDirectory(dirs->filePath);
		else ret = 0;
		auto p = dirs;
		dirs = p->next;
		delete p;
	}
	while (files) {
		WCHAR moveFromPath[MAX_PATH];
		StringCchPrintf(moveFromPath, MAX_PATH, L"%s\\%s", fromPath, files->filePath);
		WCHAR moveToPath[MAX_PATH];
		StringCchPrintf(moveToPath, MAX_PATH, L"%s\\%s", toPath, files->filePath);
		if (!MoveFileEx(moveFromPath, moveToPath, MOVEFILE_WRITE_THROUGH | MOVEFILE_FAIL_IF_NOT_TRACKABLE)) {
			std::wcout << L"파일을 이동할 수 없습니다(" << GetWinErrorStr(GetLastError()) << L"): " << moveFromPath << std::endl;
			ret = 0;
		}
		auto p = files;
		files = p->next;
		delete p;
	}
	return ret;
}

int main() {
	std::wcout.imbue(std::locale(""));
	WCHAR curPath[MAX_PATH];
	if (!GetCurrentDirectory(MAX_PATH, curPath)) {
		std::wcout << L"GetCurrentDirectory(0) 에러: " << GetWinErrorStr(GetLastError()) << std::endl;
		return 0;
	}
	std::wcout << L"경로: " << curPath << std::endl << L"정말로 실행하시겠습니까? (yes/no)" << std::endl << L">";
	WCHAR line[256];
	StringCchGets(line, 256);
	if (_wcsicmp(line, L"yes")) return 0;
	WIN32_FIND_DATA fi;
	HANDLE fh = FindFirstFile(L"*.*", &fi);
	if (fh == INVALID_HANDLE_VALUE) {
		std::wcout << L"FindFirstFile(0) 에러: " << GetWinErrorStr(GetLastError()) << std::endl;
		return 0;
	}
	MoveFileItem* dirs = NULL;
	MoveFileItem** ppd = &dirs;
	do {
		if ((fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) continue;
		if (wcscmp(fi.cFileName, L".") == 0 || wcscmp(fi.cFileName, L"..") == 0) continue;
		*ppd = new MoveFileItem();
		(*ppd)->next = NULL;
		StringCchPrintf((*ppd)->filePath, MAX_PATH, L"%s\\%s", curPath, fi.cFileName);
		ppd = &(*ppd)->next;
	} while (FindNextFile(fh, &fi) == TRUE);
	FindClose(fh);
	while (dirs) {
		if (mp(curPath, dirs->filePath, 1)) RemoveDirectory(dirs->filePath);
		auto p = dirs;
		dirs = p->next;
		delete p;
	}
	return 0;
}