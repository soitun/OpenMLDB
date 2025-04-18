# Copyright 2021 4Paradigm
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


include_directories(
    ${INCLUDE_DIRECTORIES}
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src)

add_subdirectory(statistics)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/src/proto)

set(PROTO_FILES "")
set(PROTO_CPP_FILES "")
function(compile_proto proto_name java_out_file_name)
  # will rerun if DEPENDS changed or OUTPUT not exists
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/src/proto/${proto_name}.pb.cc
      ${CMAKE_BINARY_DIR}/src/proto/${proto_name}.pb.h
      ${CMAKE_SOURCE_DIR}/java/openmldb-common/src/main/java/com/_4paradigm/openmldb/proto/${java_out_file_name}.java
    COMMAND ${Protobuf_PROTOC_EXECUTABLE} -I ${PROJECT_SOURCE_DIR}/src/proto
      --cpp_out=${CMAKE_BINARY_DIR}/src/proto
      --java_out=${CMAKE_SOURCE_DIR}/java/openmldb-common/src/main/java
      ${CMAKE_SOURCE_DIR}/src/proto/${proto_name}.proto
    DEPENDS ${CMAKE_SOURCE_DIR}/src/proto/${proto_name}.proto)
  list(APPEND PROTO_CPP_FILES ${CMAKE_BINARY_DIR}/src/proto/${proto_name}.pb.cc)
  list(APPEND PROTO_FILES
    ${CMAKE_BINARY_DIR}/src/proto/${proto_name}.pb.cc
    ${CMAKE_BINARY_DIR}/src/proto/${proto_name}.pb.h
    ${CMAKE_SOURCE_DIR}/java/openmldb-common/src/main/java/com/_4paradigm/openmldb/proto/${java_out_file_name}.java
  )
  set(PROTO_CPP_FILES ${PROTO_CPP_FILES} PARENT_SCOPE)
  set(PROTO_FILES ${PROTO_FILES} PARENT_SCOPE)
endfunction(compile_proto)

function(compile_lib LIB_NAME DIR DEPEND_FILE_LIST)
    set(FILE_STR_LIST "")
    file(GLOB_RECURSE SRC_FILES ${DIR}/*.cc)
    foreach(SRC_FILE ${SRC_FILES})
        if (NOT SRC_FILE MATCHES ".*_test.cc" AND NOT SRC_FILE MATCHES ".*_bm.cc")
            set(FILE_STR_LIST "${FILE_STR_LIST} ${SRC_FILE}")
        endif()
    endforeach()
    set(FILE_STR_LIST "${FILE_STR_LIST} ${DEPEND_FILE_LIST}")
    string(REPLACE " " ";" FILE_LIST ${FILE_STR_LIST})
    add_library(${LIB_NAME} STATIC ${FILE_LIST} $<TARGET_OBJECTS:openmldb_proto>)

    # TODO: Fix me after upgrade Boost or fix nonnull errors in Boost
    target_compile_options(${LIB_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=nonnull>
        $<$<COMPILE_LANGUAGE:C>:-Wno-error=nonnull>
    )
    target_compile_options(${LIB_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-nonnull>
        $<$<COMPILE_LANGUAGE:C>:-Wno-nonnull>
    )

endfunction(compile_lib)

set(TEST_LIBS
    openmldb_test_base apiserver nameserver tablet query_response_time openmldb_sdk
    openmldb_catalog client zk_client storage schema replica openmldb_codec base auth openmldb_proto log
    common zookeeper_mt tcmalloc_minimal ${RocksDB_LIB} ${VM_LIBS} ${LLVM_LIBS} ${ZETASQL_LIBS} ${BRPC_LIBS})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.1")
    # GNU implementation prior to 9.1 requires linking with -lstdc++fs
    list(APPEND TEST_LIBS stdc++fs)
endif()
function(compile_test_with_extra DIR DEPEND_FILE_LIST)
    file(GLOB_RECURSE SRC_FILES ${DIR}/*.cc)
    foreach(SRC_FILE ${SRC_FILES})
        if (SRC_FILE MATCHES ".*_test.cc")
            file(RELATIVE_PATH RELATIVE_TEST_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${SRC_FILE})
            get_filename_component(TEST_TARGET_DIR ${RELATIVE_TEST_PATH} DIRECTORY)
            get_filename_component(TEST_TARGET_NAME ${RELATIVE_TEST_PATH} NAME_WE)
            if(DEPEND_FILE_LIST)
                message(STATUS "add extra depend file ${DEPEND_FILE_LIST}")
            endif()
            add_executable(${TEST_TARGET_NAME} ${SRC_FILE} $<TARGET_OBJECTS:openmldb_proto> ${DEPEND_FILE_LIST})
            if (${TEST_TARGET_NAME} STREQUAL "single_tablet_test")
                add_dependencies(${TEST_TARGET_NAME} test_udf)
            endif()
            list(APPEND test_list ${TEST_TARGET_NAME})
            if (${TEST_TARGET_NAME} STREQUAL "tablet_impl_mem_test")
                target_link_libraries(${TEST_TARGET_NAME} tcmalloc ${TEST_LIBS} ${GTEST_LIBRARIES})
            else()
                target_link_libraries(${TEST_TARGET_NAME} ${TEST_LIBS} ${GTEST_LIBRARIES})
            endif()
            set_target_properties(${TEST_TARGET_NAME}
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${TEST_TARGET_DIR})
            add_test(${TEST_TARGET_NAME}
                ${CMAKE_CURRENT_BINARY_DIR}/${TEST_TARGET_DIR}/${TEST_TARGET_NAME}
                --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/${TEST_TARGET_DIR}/${TEST_TARGET_NAME}.xml)
            if (TESTING_ENABLE_STRIP)
                strip_exe(${TEST_TARGET_NAME})
            endif()
        endif()
    endforeach()
    set(test_list ${test_list} PARENT_SCOPE)
endfunction(compile_test_with_extra)
function(compile_test DIR)
    # modify the var test_list
    compile_test_with_extra(${DIR} "")
    set(test_list ${test_list} PARENT_SCOPE)
endfunction(compile_test)

compile_proto(type Type)
compile_proto(common Common)
compile_proto(tablet Tablet)
compile_proto(name_server NS)
compile_proto(sql_procedure SQLProcedure)
compile_proto(api_server ApiServer)
compile_proto(taskmanager TaskManager)
compile_proto(data_sync DataSync)

add_library(openmldb_proto STATIC ${PROTO_CPP_FILES})
# clean generated proto file, including '{cmake_source}/java/openmldb-common'
set_property(
  TARGET openmldb_proto
  APPEND
  PROPERTY ADDITIONAL_CLEAN_FILES ${PROTO_FILES}
)

add_library(openmldb_flags flags.cc)
compile_lib(auth auth "")
compile_lib(openmldb_codec codec "")
compile_lib(openmldb_catalog catalog "")
compile_lib(schema schema "")
compile_lib(client client "")
compile_lib(base base "")
compile_lib(zk_client zk "")
compile_lib(tablet tablet "")
compile_lib(nameserver nameserver "")
compile_lib(storage storage "")
compile_lib(replica replica "")
compile_lib(log log "flags.cc")
compile_lib(openmldb_sdk sdk "")
compile_lib(apiserver apiserver "")

find_package(yaml-cpp REQUIRED)
set(yaml_libs yaml-cpp)

set(BUILTIN_LIBS apiserver nameserver tablet query_response_time openmldb_sdk openmldb_catalog client zk_client replica base storage openmldb_codec schema openmldb_proto log auth ${RocksDB_LIB})
set(BIN_LIBS ${BUILTIN_LIBS}
common zookeeper_mt tcmalloc_minimal
${VM_LIBS}
${LLVM_LIBS}
${ZETASQL_LIBS}
${BRPC_LIBS})

if(TESTING_ENABLE)
    add_subdirectory(test)
    compile_test(cmd)
    compile_test(auth)
    compile_test(base)
    compile_test(codec)
    compile_test(zk)
    compile_test(tablet)
    compile_test(nameserver)
    compile_test(storage)
    compile_test(replica)
    compile_test(catalog)
    compile_test(schema)
    compile_test(log)
    compile_test(apiserver)
    # abs path
    compile_test_with_extra(datacollector ${CMAKE_CURRENT_SOURCE_DIR}/datacollector/data_collector.cc)
    add_library(test_udf SHARED examples/test_udf.cc)
endif()

add_executable(parse_log tools/parse_log.cc  $<TARGET_OBJECTS:openmldb_proto>)
add_executable(data_exporter tools/data_exporter.cc tools/log_exporter.cc tools/tablemeta_reader.cc $<TARGET_OBJECTS:openmldb_proto>)

set(LINK_LIBS log openmldb_proto base ${PROTOBUF_LIBRARY} ${GLOG_LIBRARY} ${GFLAGS_LIBRARY} ${OPENSSL_LIBRARIES} ${Z_LIBRARY} ${SNAPPY_LIBRARY} dl pthread)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND LINK_LIBS unwind)
endif()
target_link_libraries(parse_log ${LINK_LIBS})

set(EXPORTER_LIBS ${BIN_LIBS})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.1")
    # GNU implementation prior to 9.1 requires linking with -lstdc++fs
    list(APPEND EXPORTER_LIBS stdc++fs)
endif()
target_link_libraries(data_exporter ${EXPORTER_LIBS})
add_executable(openmldb cmd/openmldb.cc base/linenoise.cc)
# tablet needs std::filesystem(stdc++fs)
target_link_libraries(openmldb ${EXPORTER_LIBS})

add_executable(data_collector datacollector/data_collector_server.cc datacollector/data_collector.cc)
target_link_libraries(data_collector ${EXPORTER_LIBS})

install(
    TARGETS openmldb data_collector
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_subdirectory(sdk)

set(test_list ${test_list} PARENT_SCOPE)
