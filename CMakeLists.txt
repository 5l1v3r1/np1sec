project(np1sec)
cmake_minimum_required(VERSION 2.8.12)

option(BUILD_SHARED_LIBS "Build np1sec as a shared library" ON)
option(BUILD_TESTS "Build test components" ON)



include(FindPackageHandleStandardArgs)

find_path(GCRYPT_INCLUDE_DIR gcrypt.h)
find_library(GCRYPT_LIBRARY NAMES gcrypt libgcrypt)
set( gcrypt_FIND_REQUIRED TRUE )
find_package_handle_standard_args(gcrypt DEFAULT_MSG GCRYPT_INCLUDE_DIR GCRYPT_LIBRARY)



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${GCRYPT_INCLUDE_DIR})

add_library(np1sec
	src/debug.cc
	src/base64.cc
	src/conversation.cc
	src/conversationlist.cc
	src/crypto.cc
	src/encryptedchat.cc
	src/keyexchange.cc
	src/message.cc
	src/partition.cc
	src/room.cc
	src/session.cc
)
target_link_libraries(np1sec
	${GCRYPT_LIBRARY}
)



if(${BUILD_TESTS})
	include(test/CMakeLists.txt)
	include(test/echo_chamber/CMakeLists.txt)
endif()
