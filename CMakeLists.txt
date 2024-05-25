cmake_minimum_required(VERSION 2.8)#版本
project(sylar_server)#项目名称

include(cmake/utils.cmake)  #引用cmake/utils.cmake(获取文件路径（绝对路径——>相对路径）)      要求：CMakeLists.txt同目录存在cmake文件夹，里面要有utils.cmake文件

set(CMAKE_VERBOSE_MAKEFILE ON)#显示详细的make命令-O0 -ggdb
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++17 -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/usr/local/include) #增加yaml-cpp等头文件的路径
link_directories(/usr/local/lib)    #增加yaml-cpp等库文件的路径

set(LIB_SRC #要编译的源码路径
    sylar_server/config.cpp
    sylar_server/fiber.cpp
    sylar_server/log.cpp
    sylar_server/mutex.cpp
    sylar_server/thread.cpp
    sylar_server/util.cpp
    sylar_server/scheduler.cpp
    sylar_server/iomanager.cpp
    sylar_server/timer.cpp
    sylar_server/hook.cpp
    sylar_server/fd_manager.cpp
    sylar_server/address.cpp
    sylar_server/socket.cpp
    sylar_server/bytearray.cpp
    sylar_server/http/http.cpp
    sylar_server/http/http11_parser.rl.cpp
    sylar_server/http/httpclient_parser.rl.cpp
    sylar_server/http/http_parser.cpp
    sylar_server/tcp_server.cpp
    sylar_server/stream.cpp
    streams/socket_stream.cpp
    sylar_server/http/http_session.cpp
    sylar_server/http/http_server.cpp
    sylar_server/http/servlet.cpp
    sylar_server/uri.rl.cpp
    sylar_server/http/http_connection.cpp
    sylar_server/daemon.cpp
    sylar_server/env.cpp
    sylar_server/http/servlet_path.cpp

    sylar_server/application.cpp    #主函数

    )

add_library(sylar_server SHARED ${LIB_SRC})   #加入shared库
force_redefine_file_macro_for_sources(sylar_server)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏
#add_library(sylar_static STATIC ${LIB_SRC})  #建立静态库
#SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar")   #改静态库的库名

find_library(YAMLCPP yaml-cpp)   #用于查找yaml-cpp库。
find_library(PTHREAD pthread)

# find_package(ZLIB REQUIRED) #用于查找zlib库。
# if(ZLIB_FOUND)
#     include_directories(${ZLIB_INCLUDE_DIR})
# endif()

# find_package(OpenSSL REQUIRED)  #用于查找openssl库
# if(OPENSSL_FOUND)
#     include_directories(${OPENSSL_INCLUDE_DIR})
# endif()

set(LIBS #要依赖的库
        dl  #hook库
        sylar_server    #自定义共享库
        pthread #多线程
        yaml-cpp    #yaml-cpp
        # ${ZLIB_LIBRARIES} #zlib库
        # ${OPENSSL_LIBRARIES} #openssl库
    )
    
message("***",${LIBS})   #在日志中记录指定的消息文本

#test
add_executable(test_log tests/test_log.cpp) #生成test_log测试文件(可执行文件)
add_dependencies(test_log sylar_server) #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_log)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_log ${LIBS}) #用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_config tests/test_config.cpp)   #生成test_config测试文件(可执行文件)
add_dependencies(test_config sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_config)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_config ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_thread tests/test_thread.cpp)   #生成test_thread测试文件(可执行文件)
add_dependencies(test_thread sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_thread)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_thread ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_util tests/test_util.cpp)   #生成test_util测试文件(可执行文件)
add_dependencies(test_util sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_util)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_util ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_fiber tests/test_fiber.cpp)   #生成test_fiber测试文件(可执行文件)
add_dependencies(test_fiber sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_fiber)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_fiber ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_scheduler tests/test_scheduler.cpp)   #生成test_scheduler测试文件(可执行文件)
add_dependencies(test_scheduler sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_scheduler)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_scheduler ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_iomanager tests/test_iomanager.cpp)   #生成test_iomanager测试文件(可执行文件)
add_dependencies(test_iomanager sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_iomanager)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_iomanager ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_hook tests/test_hook.cpp)   #生成test_hook测试文件(可执行文件)
add_dependencies(test_hook sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_hook)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_hook ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_address tests/test_address.cpp)   #生成test_address测试文件(可执行文件)
add_dependencies(test_address sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_address)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_address ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_socket tests/test_socket.cpp)   #生成test_socket测试文件(可执行文件)
add_dependencies(test_socket sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_socket)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_socket ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_bytearray tests/test_bytearray.cpp)   #生成test_bytearray测试文件(可执行文件)
add_dependencies(test_bytearray sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_bytearray)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_bytearray ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_http tests/test_http.cpp)   #生成test_http测试文件(可执行文件)
add_dependencies(test_http sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_http)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_http ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_http_parser tests/test_http_parser.cpp)   #生成test_http_parser测试文件(可执行文件)
add_dependencies(test_http_parser sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_http_parser)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_http_parser ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_tcp_server tests/test_tcp_server.cpp)   #生成test_tcp_server测试文件(可执行文件)
add_dependencies(test_tcp_server sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_tcp_server)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_tcp_server ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_http_server tests/test_http_server.cpp)   #生成test_http_server测试文件(可执行文件)
add_dependencies(test_http_server sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_http_server)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_http_server ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_uri tests/test_uri.cpp)   #生成test_uri测试文件(可执行文件)
add_dependencies(test_uri sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_uri)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_uri ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_http_connection tests/test_http_connection.cpp)   #生成test_http_connection测试文件(可执行文件)
add_dependencies(test_http_connection sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_http_connection)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_http_connection ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_daemon tests/test_daemon.cpp)   #生成test_daemon测试文件(可执行文件)
add_dependencies(test_daemon sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_daemon)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_daemon ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_env tests/test_env.cpp)   #生成test_env测试文件(可执行文件)
add_dependencies(test_env sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_env)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_env ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_application tests/test_application.cpp)   #生成test_clientagreement测试文件(可执行文件)
add_dependencies(test_application sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_application)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_application ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

#examples
add_executable(echo_server examples/echo_server.cpp)   #生成echo_server测试文件(可执行文件)
add_dependencies(echo_server sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(echo_server)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(echo_server ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

#chat_room
add_executable(test_login test_chat/test_login.cpp)   #生成test_login测试文件(可执行文件)
add_dependencies(test_login sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_login)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_login ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_send test_chat/test_send.cpp)   #生成test_send测试文件(可执行文件)
add_dependencies(test_send sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_send)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_send ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_notice test_chat/test_notice.cpp)   #生成test_notice测试文件(可执行文件)
add_dependencies(test_notice sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_notice)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_notice ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_request_parser test_chat/test_request_parser.cpp)   #生成test_request_parser测试文件(可执行文件)
add_dependencies(test_request_parser sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_request_parser)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_request_parser ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_response_parser test_chat/test_response_parser.cpp)   #生成test_response_parser测试文件(可执行文件)
add_dependencies(test_response_parser sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_response_parser)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_response_parser ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_notice_parser test_chat/test_notice_parser.cpp)   #生成test_notice_parser测试文件(可执行文件)
add_dependencies(test_notice_parser sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_notice_parser)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_notice_parser ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

add_executable(test_clientagreement test_chat/test_clientagreement.cpp)   #生成test_clientagreement测试文件(可执行文件)
add_dependencies(test_clientagreement sylar_server)  #测试文件依赖 add_dependencies
force_redefine_file_macro_for_sources(test_clientagreement)    #修改 获取文件路径（绝对路径——>相对路径）   重定义__FILE__宏单元
target_link_libraries(test_clientagreement ${LIBS})#用于指定链接给定目标和/或其依赖项时要使用的库或标志

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)#输出测试文件  的路径sylar-master/bin
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)#输出库的文件路径sylar-master/lib  PROJECT_SOURCE_DIR预定义变量
