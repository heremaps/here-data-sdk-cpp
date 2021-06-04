/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#include "Helpers.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <stdlib.h>
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>
#include <vector>
#else
#include <errno.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {
#if defined(_WIN32) && !defined(__MINGW32__)

bool MakeDirectoryAndContentReadonlyWin(const TCHAR* utfdirPath,
                                        bool readonly) {
  HANDLE hFind;  // file handle
  WIN32_FIND_DATA FindFileData;

  if (utfdirPath == 0 || *utfdirPath == '\0') {
    return false;
  }

  auto dir_attributes = ::GetFileAttributes(utfdirPath);
  if (dir_attributes == INVALID_FILE_ATTRIBUTES) {
    return false;
  }

  bool is_directory =
      (dir_attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
  if (!is_directory) {
    return false;
  }

  if (readonly) {
    SetFileAttributes(utfdirPath, dir_attributes | FILE_ATTRIBUTE_READONLY);
  } else {
    SetFileAttributes(utfdirPath, dir_attributes & ~FILE_ATTRIBUTE_READONLY);
  }

  TCHAR dirPath[MAX_PATH];
  TCHAR filename[MAX_PATH];

  StringCchCopy(dirPath, _countof(dirPath), utfdirPath);
  StringCchCat(dirPath, _countof(dirPath),
               TEXT("\\*"));  // searching all files
  StringCchCopy(filename, _countof(filename), utfdirPath);
  StringCchCat(filename, _countof(filename), TEXT("\\"));

  bool bSearch = true;

  hFind = FindFirstFile(dirPath, &FindFileData);  // find the first file
  if (hFind != INVALID_HANDLE_VALUE) {
    StringCchCopy(dirPath, _countof(dirPath), filename);

    do {
      if ((lstrcmp(FindFileData.cFileName, TEXT(".")) == 0) ||
          (lstrcmp(FindFileData.cFileName, TEXT("..")) == 0)) {
        continue;
      }

      StringCchCopy(filename, _countof(filename), dirPath);
      StringCchCat(filename, _countof(filename), FindFileData.cFileName);
      if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        // we have found a directory, recurse
        if (!MakeDirectoryAndContentReadonlyWin(filename, readonly)) {
          bSearch = false;
          break;
        }
      } else {
        DWORD attribs = GetFileAttributes(filename);
        if (readonly) {
          SetFileAttributes(filename, attribs | FILE_ATTRIBUTE_READONLY);
        } else {
          SetFileAttributes(filename, attribs & ~FILE_ATTRIBUTE_READONLY);
        }
      }
    } while (bSearch &&
             FindNextFile(hFind, &FindFileData));  // until we finds an entry

    if (bSearch) {
      if (GetLastError() != ERROR_NO_MORE_FILES)  // no more files there
      {
        bSearch = false;
      }
    }

    FindClose(hFind);  // closing file handle
  } else {  // Because on WinCE, FindFirstFile will fail for empty folder.
    // Other than that, we should not try to remove the directory then.
    if (GetLastError() != ERROR_NO_MORE_FILES)  // no more files there
    {
      bSearch = false;
    }
  }

  return bSearch;
}

#else

static constexpr mode_t WRITE_PERMISIONS = S_IWUSR | S_IWGRP | S_IWOTH;

int make_readonly_callback(const char* file, const struct stat* stat, int flag,
                           struct FTW* /*buf*/) {
  if (flag == (FTW_F | FTW_D)) {
    return chmod(file, stat->st_mode & ~WRITE_PERMISIONS);
  } else {
    return 0;
  }
}

int make_read_write_callback(const char* file, const struct stat* stat,
                             int flag, struct FTW* /*buf*/) {
  if (flag == (FTW_F | FTW_D)) {
    return chmod(file, stat->st_mode | WRITE_PERMISIONS);
  } else {
    return 0;
  }
}

#endif
}  // namespace

namespace helpers {
bool MakeDirectoryAndContentReadonly(const std::string& path, bool readonly) {
#if defined(_WIN32) && !defined(__MINGW32__)
#ifdef _UNICODE

  int size_needed = MultiByteToWideChar(CP_ACP, 0, &path[0], path.size(), NULL, 0);
  std::wstring wstr_path(size_needed, 0);
  MultiByteToWideChar(CP_ACP, 0, &path[0], path.size(), &wstr_path[0], size_needed);
  const TCHAR* n_path = wstr_path.c_str();
#else
  const TCHAR* n_path = path.c_str();
#endif  // _UNICODE

  return MakeDirectoryAndContentReadonlyWin(n_path, readonly);

#else

  auto callback = readonly ? make_readonly_callback : make_read_write_callback;
  return nftw(path.c_str(), callback, 10, FTW_DEPTH | FTW_PHYS) == 0;

#endif
}
}  // namespace helpers
