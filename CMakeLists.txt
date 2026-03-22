cmake_minimum_required(VERSION 3.10)
# Подавляем ворнинг о старом способе поиска
cmake_policy(SET CMP0167 OLD)
project(Lab1_p2p)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED)

add_executable(server server/main.cpp)
target_link_libraries(server PRIVATE Boost::headers)

add_executable(client client/main.cpp)
target_link_libraries(client PRIVATE Boost::headers)

add_executable(third third/main.cpp)
target_link_libraries(third PRIVATE Boost::headers)

add_executable(peer1 peer1/main.cpp)
target_link_libraries(peer1 PRIVATE Boost::headers)

add_executable(tracker tracker/main.cpp)
target_link_librares(tracker PRIVATE Boost::headers)
