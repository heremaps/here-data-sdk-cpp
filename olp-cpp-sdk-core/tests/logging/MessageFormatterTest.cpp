/*
 * Copyright (C) 2019 HERE Europe B.V.
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

#include <gtest/gtest.h>

#include <olp/core/logging/MessageFormatter.h>

namespace {

using namespace olp::logging;

TEST(MessageFormatterTest, ElementConstructors) {
  MessageFormatter::Element string_element(
      MessageFormatter::ElementType::String);
  EXPECT_EQ(MessageFormatter::ElementType::String, string_element.type);
  EXPECT_EQ("", string_element.format);
  EXPECT_EQ(0, string_element.limit);

  MessageFormatter::Element level_element(MessageFormatter::ElementType::Level);
  EXPECT_EQ(MessageFormatter::ElementType::Level, level_element.type);
  EXPECT_EQ("%s", level_element.format);
  EXPECT_EQ(0, level_element.limit);

  MessageFormatter::Element tag_element(MessageFormatter::ElementType::Tag);
  EXPECT_EQ(MessageFormatter::ElementType::Tag, tag_element.type);
  EXPECT_EQ("%s", tag_element.format);
  EXPECT_EQ(0, tag_element.limit);

  MessageFormatter::Element message_element(
      MessageFormatter::ElementType::Message);
  EXPECT_EQ(MessageFormatter::ElementType::Message, message_element.type);
  EXPECT_EQ("%s", message_element.format);
  EXPECT_EQ(0, message_element.limit);

  MessageFormatter::Element file_element(MessageFormatter::ElementType::File);
  EXPECT_EQ(MessageFormatter::ElementType::File, file_element.type);
  EXPECT_EQ("%s", file_element.format);
  EXPECT_EQ(0, file_element.limit);

  MessageFormatter::Element line_element(MessageFormatter::ElementType::Line);
  EXPECT_EQ(MessageFormatter::ElementType::Line, line_element.type);
  EXPECT_EQ("%u", line_element.format);
  EXPECT_EQ(0, line_element.limit);

  MessageFormatter::Element function_element(
      MessageFormatter::ElementType::Function);
  EXPECT_EQ(MessageFormatter::ElementType::Function, function_element.type);
  EXPECT_EQ("%s", function_element.format);
  EXPECT_EQ(0, function_element.limit);

  MessageFormatter::Element full_function_element(
      MessageFormatter::ElementType::FullFunction);
  EXPECT_EQ(MessageFormatter::ElementType::FullFunction,
            full_function_element.type);
  EXPECT_EQ("%s", full_function_element.format);
  EXPECT_EQ(0, full_function_element.limit);

  MessageFormatter::Element time_element(MessageFormatter::ElementType::Time);
  EXPECT_EQ(MessageFormatter::ElementType::Time, time_element.type);
  EXPECT_EQ("%Y-%m-%d %H:%M:%S", time_element.format);
  EXPECT_EQ(0, time_element.limit);

  MessageFormatter::Element time_ms_element(
      MessageFormatter::ElementType::TimeMs);
  EXPECT_EQ(MessageFormatter::ElementType::TimeMs, time_ms_element.type);
  EXPECT_EQ("%u", time_ms_element.format);
  EXPECT_EQ(0, time_ms_element.limit);

  MessageFormatter::Element thread_id_element(
      MessageFormatter::ElementType::ThreadId);
  EXPECT_EQ(MessageFormatter::ElementType::ThreadId, thread_id_element.type);
  EXPECT_EQ("%lu", thread_id_element.format);
  EXPECT_EQ(0, thread_id_element.limit);

  MessageFormatter::Element arbitrary_element(
      MessageFormatter::ElementType::File, "%30s", -30);
  EXPECT_EQ(MessageFormatter::ElementType::File, arbitrary_element.type);
  EXPECT_EQ("%30s", arbitrary_element.format);
  EXPECT_EQ(-30, arbitrary_element.limit);
}

TEST(MessageFormatterTest, ElementMove) {
  MessageFormatter::Element element(MessageFormatter::ElementType::File, "%30s",
                                    -30);

  MessageFormatter::Element move_constructed(std::move(element));
  EXPECT_EQ(MessageFormatter::ElementType::File, move_constructed.type);
  EXPECT_EQ("%30s", move_constructed.format);
  EXPECT_EQ(-30, move_constructed.limit);
  EXPECT_TRUE(element.format.empty());

  MessageFormatter::Element move_assigned(
      MessageFormatter::ElementType::String);
  move_assigned = std::move(move_constructed);
  EXPECT_EQ(MessageFormatter::ElementType::File, move_assigned.type);
  EXPECT_EQ("%30s", move_assigned.format);
  EXPECT_EQ(-30, move_assigned.limit);
  EXPECT_TRUE(move_constructed.format.empty());
}

TEST(MessageFormatterTest, ElementEquality) {
  MessageFormatter::Element element1(MessageFormatter::ElementType::File,
                                     "%30s", -30);
  MessageFormatter::Element element2(element1);

  EXPECT_TRUE(element1 == element2);
  EXPECT_FALSE(element1 != element2);

  element2.type = MessageFormatter::ElementType::Line;
  EXPECT_FALSE(element1 == element2);
  EXPECT_TRUE(element1 != element2);

  element2 = element1;
  element2.format = "%u";
  EXPECT_FALSE(element1 == element2);
  EXPECT_TRUE(element1 != element2);

  element2 = element1;
  element2.limit = 20;
  EXPECT_FALSE(element1 == element2);
  EXPECT_TRUE(element1 != element2);
}

TEST(MessageFormatterTest, DefaultLevelMap) {
  const MessageFormatter::LevelNameMap& map =
      MessageFormatter::defaultLevelNameMap();
  EXPECT_EQ("[TRACE]", map[static_cast<std::size_t>(Level::Trace)]);
  EXPECT_EQ("[DEBUG]", map[static_cast<std::size_t>(Level::Debug)]);
  EXPECT_EQ("[INFO]", map[static_cast<std::size_t>(Level::Info)]);
  EXPECT_EQ("[WARN]", map[static_cast<std::size_t>(Level::Warning)]);
  EXPECT_EQ("[ERROR]", map[static_cast<std::size_t>(Level::Error)]);
  EXPECT_EQ("[FATAL]", map[static_cast<std::size_t>(Level::Fatal)]);
}

TEST(MessageFormatterTest, DefaultConstructor) {
  MessageFormatter formatter;
  EXPECT_TRUE(formatter.getElements().empty());
  EXPECT_EQ(MessageFormatter::defaultLevelNameMap(),
            formatter.getLevelNameMap());
  EXPECT_EQ(MessageFormatter::Timezone::Local, formatter.getTimezone());
}

TEST(MessageFormatterTest, Constructor) {
  static const std::vector<MessageFormatter::Element> elements = {
      MessageFormatter::Element(MessageFormatter::ElementType::String, "LOG: "),
      MessageFormatter::Element(MessageFormatter::ElementType::Level, "%s "),
      MessageFormatter::Element(MessageFormatter::ElementType::Tag, "%s - "),
      MessageFormatter::Element(MessageFormatter::ElementType::File,
                                "%30s:", -30),
      MessageFormatter::Element(MessageFormatter::ElementType::Line, "%5u "),
      MessageFormatter::Element(MessageFormatter::ElementType::Time,
                                "[%H:%M] "),
      MessageFormatter::Element(MessageFormatter::ElementType::Message)};

  static const MessageFormatter::LevelNameMap level_name_map = {
      {{"Trace"}, {"Debug"}, {"Info"}, {"Warning"}, {"Error"}, {"Fatal"}}};

  MessageFormatter formatter(elements, level_name_map,
                             MessageFormatter::Timezone::Utc);
  EXPECT_EQ(elements, formatter.getElements());
  EXPECT_EQ(level_name_map, formatter.getLevelNameMap());
  EXPECT_EQ(MessageFormatter::Timezone::Utc, formatter.getTimezone());
}

TEST(MessageFormatterTest, CreateDefault) {
  MessageFormatter formatter = MessageFormatter::createDefault();
  EXPECT_FALSE(formatter.getElements().empty());
  EXPECT_EQ(MessageFormatter::defaultLevelNameMap(),
            formatter.getLevelNameMap());
  EXPECT_EQ(MessageFormatter::Timezone::Local, formatter.getTimezone());
}

TEST(MessageFormatterTest, SetElements) {
  MessageFormatter formatter;

  static const std::vector<MessageFormatter::Element> elements = {
      MessageFormatter::Element(MessageFormatter::ElementType::String, "LOG: "),
      MessageFormatter::Element(MessageFormatter::ElementType::Level, "%s "),
      MessageFormatter::Element(MessageFormatter::ElementType::Tag, "%s - "),
      MessageFormatter::Element(MessageFormatter::ElementType::File,
                                "%30s:", -30),
      MessageFormatter::Element(MessageFormatter::ElementType::Line, "%5u "),
      MessageFormatter::Element(MessageFormatter::ElementType::Time,
                                "[%H:%M] "),
      MessageFormatter::Element(MessageFormatter::ElementType::Message)};

  EXPECT_NE(elements, formatter.getElements());
  formatter.setElements(elements);
  EXPECT_EQ(elements, formatter.getElements());
}

TEST(MessageFormatterTest, SetLevelNameMap) {
  MessageFormatter formatter;

  static const MessageFormatter::LevelNameMap level_name_map = {
      {{"Trace"}, {"Debug"}, {"Info"}, {"Warning"}, {"Error"}, {"Fatal"}}};

  EXPECT_NE(level_name_map, formatter.getLevelNameMap());
  formatter.setLevelNameMap(level_name_map);
  EXPECT_EQ(level_name_map, formatter.getLevelNameMap());
}

TEST(MessageFormatterTest, SetTimezone) {
  MessageFormatter formatter;
  EXPECT_EQ(MessageFormatter::Timezone::Local, formatter.getTimezone());
  formatter.setTimezone(MessageFormatter::Timezone::Utc);
  EXPECT_EQ(MessageFormatter::Timezone::Utc, formatter.getTimezone());
}

TEST(MessageFormatterTest, Format) {
  static const std::vector<MessageFormatter::Element> elements = {
      MessageFormatter::Element(MessageFormatter::ElementType::String, "Test "),
      MessageFormatter::Element(MessageFormatter::ElementType::Level, "%s "),
      MessageFormatter::Element(MessageFormatter::ElementType::Tag, "%s - "),
      MessageFormatter::Element(MessageFormatter::ElementType::File, "%s:"),
      MessageFormatter::Element(MessageFormatter::ElementType::Line, "%u:"),
      MessageFormatter::Element(MessageFormatter::ElementType::Function,
                                "%s(): "),
      MessageFormatter::Element(MessageFormatter::ElementType::Time,
                                "[%H:%M:%S:"),
      MessageFormatter::Element(MessageFormatter::ElementType::TimeMs,
                                "%03u] "),
      MessageFormatter::Element(MessageFormatter::ElementType::Message),
      MessageFormatter::Element(MessageFormatter::ElementType::String, " Log")};

  static const MessageFormatter::LevelNameMap level_name_map = {
      {{"Trace"}, {"Debug"}, {"Info"}, {"Warning"}, {"Error"}, {"Fatal"}}};

  MessageFormatter formatter(elements, level_name_map,
                             MessageFormatter::Timezone::Utc);

  LogMessage message;
  message.level = Level::Info;
  message.tag = "tag";
  message.message = "message";
  message.file = "file.cpp";
  message.line = 1234;
  message.function = "function";
  message.time = std::chrono::time_point<std::chrono::system_clock>(
      std::chrono::milliseconds(12345012));

  EXPECT_EQ(
      "Test Info tag - file.cpp:1234:function(): [03:25:45:012] message Log",
      formatter.format(message));

  message.tag = "";
  EXPECT_EQ("Test Info file.cpp:1234:function(): [03:25:45:012] message Log",
            formatter.format(message));

  message.tag = nullptr;
  EXPECT_EQ("Test Info file.cpp:1234:function(): [03:25:45:012] message Log",
            formatter.format(message));
}

TEST(MessageFormatterTest, ThreadId) {
  static const unsigned long kThreadId1 = 1;
  static const unsigned long kThreadId2 = 2;

  MessageFormatter formatter(
      {MessageFormatter::Element(MessageFormatter::ElementType::ThreadId)});
  LogMessage message;
  message.level = Level::Info;
  message.tag = "tag";
  message.message = "message";
  message.file = "file.cpp";
  message.line = 1234;
  message.function = "function";
  message.threadId = kThreadId1;

  std::string thread1_message = formatter.format(message);

  message.threadId = kThreadId2;
  std::string thread2_message = formatter.format(message);

  EXPECT_EQ(format("%lu", kThreadId1), thread1_message);
  EXPECT_EQ(format("%lu", kThreadId2), thread2_message);
}

TEST(MessageFormatterTest, TagLimits) {
  std::vector<MessageFormatter::Element> elements = {
      MessageFormatter::Element(MessageFormatter::ElementType::Tag, "%s")};

  MessageFormatter formatter(elements);

  LogMessage message;
  message.level = Level::Info;
  message.tag = "I am an arbitrary string";
  message.message = "message";
  message.file = "file.cpp";
  message.line = 1234;
  message.function = "function";
  message.threadId = 987546;

  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = -50;
  formatter.setElements(elements);
  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = 50;
  formatter.setElements(elements);
  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = -10;
  formatter.setElements(elements);
  EXPECT_EQ("... string", formatter.format(message));

  elements[0].limit = 10;
  formatter.setElements(elements);
  EXPECT_EQ("I am an...", formatter.format(message));

  elements[0].limit = -3;
  formatter.setElements(elements);
  EXPECT_EQ("ing", formatter.format(message));

  elements[0].limit = 3;
  formatter.setElements(elements);
  EXPECT_EQ("I a", formatter.format(message));
}

TEST(MessageFormatterTest, MessageLimits) {
  std::vector<MessageFormatter::Element> elements = {
      MessageFormatter::Element(MessageFormatter::ElementType::Message, "%s")};

  MessageFormatter formatter(elements);

  LogMessage message;
  message.level = Level::Info;
  message.tag = "tag";
  message.message = "I am an arbitrary string";
  message.file = "file.cpp";
  message.line = 1234;
  message.function = "function";
  message.threadId = 5321456;

  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = -50;
  formatter.setElements(elements);
  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = 50;
  formatter.setElements(elements);
  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = -10;
  formatter.setElements(elements);
  EXPECT_EQ("... string", formatter.format(message));

  elements[0].limit = 10;
  formatter.setElements(elements);
  EXPECT_EQ("I am an...", formatter.format(message));

  elements[0].limit = -3;
  formatter.setElements(elements);
  EXPECT_EQ("ing", formatter.format(message));

  elements[0].limit = 3;
  formatter.setElements(elements);
  EXPECT_EQ("I a", formatter.format(message));
}

TEST(MessageFormatterTest, FileLimits) {
  std::vector<MessageFormatter::Element> elements = {
      MessageFormatter::Element(MessageFormatter::ElementType::File, "%s")};

  MessageFormatter formatter(elements);

  LogMessage message;
  message.level = Level::Info;
  message.tag = "tag";
  message.message = "message";
  message.file = "I am an arbitrary string";
  message.line = 1234;
  message.function = "function";
  message.threadId = 9873122;

  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = -50;
  formatter.setElements(elements);
  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = 50;
  formatter.setElements(elements);
  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = -10;
  formatter.setElements(elements);
  EXPECT_EQ("... string", formatter.format(message));

  elements[0].limit = 10;
  formatter.setElements(elements);
  EXPECT_EQ("I am an...", formatter.format(message));

  elements[0].limit = -3;
  formatter.setElements(elements);
  EXPECT_EQ("ing", formatter.format(message));

  elements[0].limit = 3;
  formatter.setElements(elements);
  EXPECT_EQ("I a", formatter.format(message));
}

TEST(MessageFormatterTest, FunctionLimits) {
  std::vector<MessageFormatter::Element> elements = {
      MessageFormatter::Element(MessageFormatter::ElementType::Function, "%s")};

  MessageFormatter formatter(elements);

  LogMessage message;
  message.level = Level::Info;
  message.tag = "tag";
  message.message = "message";
  message.file = "file.cpp";
  message.line = 1234;
  message.function = "I am an arbitrary string";
  message.threadId = 8974313;

  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = -50;
  formatter.setElements(elements);
  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = 50;
  formatter.setElements(elements);
  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = -10;
  formatter.setElements(elements);
  EXPECT_EQ("... string", formatter.format(message));

  elements[0].limit = 10;
  formatter.setElements(elements);
  EXPECT_EQ("I am an...", formatter.format(message));

  elements[0].limit = -3;
  formatter.setElements(elements);
  EXPECT_EQ("ing", formatter.format(message));

  elements[0].limit = 3;
  formatter.setElements(elements);
  EXPECT_EQ("I a", formatter.format(message));
}

TEST(MessageFormatterTest, FullFunctionLimits) {
  std::vector<MessageFormatter::Element> elements = {MessageFormatter::Element(
      MessageFormatter::ElementType::FullFunction, "%s")};

  MessageFormatter formatter(elements);

  LogMessage message;
  message.level = Level::Info;
  message.tag = "tag";
  message.message = "message";
  message.file = "file.cpp";
  message.line = 1234;
  message.fullFunction = "I am an arbitrary string";
  message.threadId = 3246573;

  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = -50;
  formatter.setElements(elements);
  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = 50;
  formatter.setElements(elements);
  EXPECT_EQ("I am an arbitrary string", formatter.format(message));

  elements[0].limit = -10;
  formatter.setElements(elements);
  EXPECT_EQ("... string", formatter.format(message));

  elements[0].limit = 10;
  formatter.setElements(elements);
  EXPECT_EQ("I am an...", formatter.format(message));

  elements[0].limit = -3;
  formatter.setElements(elements);
  EXPECT_EQ("ing", formatter.format(message));

  elements[0].limit = 3;
  formatter.setElements(elements);
  EXPECT_EQ("I a", formatter.format(message));
}

}  // namespace
