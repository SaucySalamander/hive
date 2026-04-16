cmake_minimum_required(VERSION 3.22)

project(hive VERSION 0.1.0 LANGUAGES C)

include(GNUInstallDirs)
include(CheckIncludeFile)

option(HIVE_ENABLE_TUI "Build the ncurses TUI" ON)
option(HIVE_ENABLE_API "Build the optional libuv/cJSON HTTP API" OFF)
option(HIVE_ENABLE_SYSLOG "Enable syslog logging when available" ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

check_include_file(argp.h HIVE_HAVE_ARGP_H)
check_include_file(syslog.h HIVE_HAVE_SYSLOG_H)

set(HIVE_HAVE_NCURSES 0)
set(HIVE_HAVE_LIBUV 0)
set(HIVE_HAVE_CJSON 0)
set(HIVE_LIBUV_TARGET "")
set(HIVE_CJSON_TARGET "")

if(HIVE_ENABLE_TUI)
  find_package(Curses QUIET)
  if(CURSES_FOUND)
    set(HIVE_HAVE_NCURSES 1)
  endif()
endif()

if(HIVE_ENABLE_API)
  find_package(PkgConfig QUIET)

  find_package(uv QUIET CONFIG)
  if(TARGET uv::uv)
    set(HIVE_LIBUV_TARGET uv::uv)
    set(HIVE_HAVE_LIBUV 1)
  elseif(TARGET uv)
    set(HIVE_LIBUV_TARGET uv)
    set(HIVE_HAVE_LIBUV 1)
  elseif(PkgConfig_FOUND)
    pkg_check_modules(PC_LIBUV QUIET IMPORTED_TARGET libuv)
    if(PC_LIBUV_FOUND)
      set(HIVE_LIBUV_TARGET PkgConfig::PC_LIBUV)
      set(HIVE_HAVE_LIBUV 1)
    endif()
  endif()

  find_package(cJSON QUIET CONFIG)
  if(TARGET cJSON::cJSON)
    set(HIVE_CJSON_TARGET cJSON::cJSON)
    set(HIVE_HAVE_CJSON 1)
  elseif(TARGET cjson::cjson)
    set(HIVE_CJSON_TARGET cjson::cjson)
    set(HIVE_HAVE_CJSON 1)
  elseif(PkgConfig_FOUND)
    pkg_check_modules(PC_CJSON QUIET IMPORTED_TARGET cjson)
    if(PC_CJSON_FOUND)
      set(HIVE_CJSON_TARGET PkgConfig::PC_CJSON)
      set(HIVE_HAVE_CJSON 1)
    endif()
  endif()
endif()

if(HIVE_ENABLE_TUI AND NOT HIVE_HAVE_NCURSES)
  message(STATUS "hive: ncurses not found, building the TUI stub")
endif()

if(HIVE_ENABLE_API AND NOT (HIVE_HAVE_LIBUV AND HIVE_HAVE_CJSON))
  message(STATUS "hive: libuv/cJSON not fully found, building the API stub")
endif()

set(HIVE_SOURCES
  src/main.c
  src/common/strings.c
  src/core/types.c
  src/logging/logger.c
  src/inference/adapter.c
  src/core/session.c
  src/core/evaluator.c
  src/core/reflexion.c
  src/core/agent.c
  src/core/orchestrator.c
  src/core/planner.c
  src/core/coder.c
  src/core/tester.c
  src/core/verifier.c
  src/core/editor.c
  src/tools/registry.c
  src/core/state_machine.c
  src/core/runtime.c
  src/cli/cli.c
  src/tui/tui.c
  src/api/server.c
)

add_executable(hive ${HIVE_SOURCES})

target_include_directories(hive PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(hive PRIVATE
  _POSIX_C_SOURCE=200809L
  HIVE_HAVE_ARGP=$<BOOL:${HIVE_HAVE_ARGP_H}>
  HIVE_HAVE_SYSLOG=$<BOOL:${HIVE_HAVE_SYSLOG_H}>
  HIVE_HAVE_NCURSES=${HIVE_HAVE_NCURSES}
  HIVE_HAVE_LIBUV=${HIVE_HAVE_LIBUV}
  HIVE_HAVE_CJSON=${HIVE_HAVE_CJSON}
  HIVE_ENABLE_SYSLOG=$<BOOL:${HIVE_ENABLE_SYSLOG}>
)

if(MSVC)
  target_compile_options(hive PRIVATE /W4 /WX)
else()
  target_compile_options(hive PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

target_link_libraries(hive PRIVATE Threads::Threads)

if(HIVE_HAVE_NCURSES)
  if(TARGET Curses::Curses)
    target_link_libraries(hive PRIVATE Curses::Curses)
  else()
    target_include_directories(hive PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(hive PRIVATE ${CURSES_LIBRARIES})
  endif()
endif()

if(HIVE_HAVE_LIBUV)
  target_link_libraries(hive PRIVATE ${HIVE_LIBUV_TARGET})
endif()

if(HIVE_HAVE_CJSON)
  target_link_libraries(hive PRIVATE ${HIVE_CJSON_TARGET})
endif()

install(TARGETS hive RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
