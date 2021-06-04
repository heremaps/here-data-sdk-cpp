/*
 * Copyright (C) 2021 HERE Europe B.V.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <olp/core/utils/Dir.h>
#include <fstream>
#if !defined(_WIN32) || defined(__MINGW32__)
#include <unistd.h>
#else
#include <windows.h>
#endif

namespace {
using Dir = olp::utils::Dir;

#if defined(_WIN32) && !defined(__MINGW32__)
const char kSeparator[] = "\\";
#else
const char kSeparator[] = "/";
#endif

void CreateFile(const std::string& path, size_t size) {
  std::ofstream file(path.c_str(), std::ios::out | std::ios::binary);
  file.seekp(size - 1);
  file.write("", 1);
  file.close();
}

void CreateDirectory(const std::string& path) { Dir::Create(path); }

void CreateSymLink(const std::string& to, const std::string& link) {
#if !defined(_WIN32) || defined(__MINGW32__)
  symlink(to.c_str(), link.c_str());
#else
  CreateSymbolicLink(to.c_str(), link.c_str(), 0);
#endif
}

std::string PathBuild(std::string arg) { return arg; }

template <typename... Ts>
std::string PathBuild(std::string arg1, std::string arg2, Ts... args) {
  return PathBuild(std::string(arg1) + kSeparator + std::string(arg2), args...);
}

bool SetRights(const std::string& path, bool readonly) {
#if !defined(_WIN32) || defined(__MINGW32__)
  mode_t mode;
  if (readonly) {
    mode = S_IRUSR | S_IRGRP | S_IROTH;
  } else {
    mode = S_IRUSR | S_IWUSR | S_IXUSR;
  }
  mode = mode | S_IRGRP | S_IROTH;
  return chmod(path.c_str(), mode) == 0;
#else
  DWORD attributes = GetFileAttributes(path.c_str());
  attributes = readonly ? attributes | FILE_ATTRIBUTE_READONLY
                        : attributes & ~FILE_ATTRIBUTE_READONLY;
  BOOL result = SetFileAttributesA(path.c_str(), attributes);
  return result != 0;
#endif
}

}  // namespace

TEST(DirTest, CheckDirSize) {
  std::string path = PathBuild(Dir::TempDirectory(), "temporary_test_dir");
  Dir::Remove(path);
  CreateDirectory(path);
  {
    SCOPED_TRACE("Single file");
    CreateFile(PathBuild(path, "file1"), 10);
    EXPECT_EQ(Dir::Size(path), 10u);
  }
  {
    SCOPED_TRACE("First level subdirectory");
    CreateDirectory(PathBuild(path, "sub"));
    CreateFile(PathBuild(path, "sub", "sub_file1"), 10);
    EXPECT_EQ(Dir::Size(path), 20u);
  }
  {
    SCOPED_TRACE("Second level subdirectory");
    CreateDirectory(PathBuild(path, "sub", "subsub"));
    CreateFile(PathBuild(path, "sub", "subsub", "subsub_file1"), 10);
    EXPECT_EQ(Dir::Size(path), 30u);
  }
  {
    SCOPED_TRACE("Symbolic links to directory and file");
    CreateSymLink("sub", PathBuild(path, "sub_lnk"));
    CreateSymLink("file1", PathBuild(path, "file1_lnk"));
    EXPECT_EQ(Dir::Size(path), 30u);
  }
  {
    SCOPED_TRACE("Second file in directory and subdirectories");
    CreateFile(PathBuild(path, "file2"), 10);
    CreateFile(PathBuild(path, "sub", "sub_file2"), 10);
    CreateFile(PathBuild(path, "sub", "subsub", "subsub_file2"), 10);
    EXPECT_EQ(Dir::Size(path), 60u);
  }
  Dir::Remove(path);
}

TEST(DirTest, IsReadOnlyTest) {
  std::string path = PathBuild(Dir::TempDirectory(), "temporary_test_dir");
  Dir::Remove(path);
  CreateDirectory(path);
  {
    SCOPED_TRACE("Directory access right test");
    EXPECT_FALSE(Dir::IsReadOnly(path));
  }
  {
    SCOPED_TRACE("File access right test");
    auto file_path = PathBuild(path, "file1");
    CreateFile(file_path, 1);
    EXPECT_FALSE(Dir::IsReadOnly(file_path));
  }
  {
    SCOPED_TRACE("Check read only directory");
    ASSERT_TRUE(SetRights(path, true));
    EXPECT_TRUE(Dir::IsReadOnly(path));
    ASSERT_TRUE(SetRights(path, false));
  }
  {
    SCOPED_TRACE("Check read only file");
    auto file_path = PathBuild(path, "file1");
    CreateFile(file_path, 1);
    ASSERT_TRUE(SetRights(file_path, true));
    EXPECT_TRUE(Dir::IsReadOnly(file_path));
    ASSERT_TRUE(SetRights(file_path, false));
  }
}
