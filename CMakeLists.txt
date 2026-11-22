cmake_minimum_required(VERSION 3.16)
project(ChatServer LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Biblioteka
add_library(common_lib

        src/common/config.cpp
        src/common/net_utils.cpp
        src/common/protocol.cpp
        src/common/time_utils.cpp
        src/common/helpers.cpp

)
target_include_directories(common_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(client_lib

        src/client/command_parser.cpp
        src/client/connector.cpp
        src/client/history.cpp
        src/client/receiver.cpp
        
)
target_link_libraries(client_lib PUBLIC common_lib)
target_include_directories(client_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(server_lib

        src/server/client_session.cpp
        src/server/dispatcher.cpp
        src/server/file_transfer.cpp
        src/server/listener.cpp
        src/server/logger.cpp
        src/server/room_manager.cpp
        src/server/user_registry.cpp

)
target_link_libraries(server_lib PUBLIC common_lib)
target_include_directories(server_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
# Aplikacja 
add_executable(server src/server/main.cpp)
target_link_libraries(server PRIVATE server_lib)

add_executable(client src/client/main.cpp)
target_link_libraries(client PRIVATE client_lib)



#install(TARGETS serwer-klient RUNTIME DESTINATION bin)




