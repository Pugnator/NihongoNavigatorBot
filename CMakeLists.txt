cmake_minimum_required(VERSION 3.21)

project(wakaBOT LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(CURL 7.58.0)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CODE_COVERAGE "-fprofile-arcs -ftest-coverage")
    set(LIBCODE_COVERAGE "-lgcov --coverage")
    set(OPTIM "-ggdb -Og")
    add_definitions(-DWAKA_DEBUG)
else()
    set(OPTIM "-Ofast")
    add_definitions(-DWAKA_RELEASE)
endif()

set(CFLAGS "${CFLAGS} ${OPTIM}")
set(CXXFLAGS "${CXXFLAGS} ${OPTIM}")

if(PRODUCTION_BUILD)
    if(EXISTS "${CMAKE_SOURCE_DIR}/token_prod.txt")
        file(READ "token_prod.txt" TOKEN_CONTENT)
    else()
        set(TOKEN_CONTENT "EMPTY_TOKEN")
    endif()
else()
    if(EXISTS "${CMAKE_SOURCE_DIR}/token.txt")
        file(READ "token.txt" TOKEN_CONTENT)
    else()
        set(TOKEN_CONTENT "EMPTY_TOKEN")
    endif()
endif()

configure_file("config.h.in" "config.h")

add_definitions(-DWAKABOT_TOKEN="${TOKEN_CONTENT}")


add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)
set(TGBOT_DISABLE_NAGLES_ALGORITHM ON)
add_subdirectory(tgbot-cpp)

set(CPPSRC
  wakabot.cc
  botcommander.cc
  callback.cc
  commands.cc
  commands/command_global.cc
  commands/command_quiz.cc
  commands/quiz/listening.cc
  commands/quiz/word_meaning.cc
  commands/quiz/word_reading.cc
  commands/quiz/kana_reading.cc
  commands/quiz/numerals.cc
  commands/command_search.cc
  commands/command_explain.cc
  eventsmanager.cc
  usermanager.cc
)

add_executable(wakaBOT ${CPPSRC})
set_target_properties(wakaBOT PROPERTIES OUTPUT_NAME "wakabot")