############################################################################
# Copyright 2019-2030 Bouffalolab team
#
# Copyright 2021 Salieri <f.yu@verimake.com>
# 
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
###########################################################################

cmake_minimum_required(VERSION 3.3.2)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 0.1)
set(CMAKE_SYSTEM_PROCESSOR RISCV)
if(MINGW OR CYGWIN OR WIN32)
    set(WHERE_CMD where)
    set(TOOLCHAIN_SUFFIX ".exe")
elseif(UNIX OR APPLE)
    set(WHERE_CMD which)
    set(TOOLCHAIN_SUFFIX "")
endif()
set(TOOLCHAIN_PREFIX riscv-none-embed-)
execute_process(
  COMMAND ${WHERE_CMD} ${TOOLCHAIN_PREFIX}gcc
  OUTPUT_VARIABLE TOOLCHAIN_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "TOOLCHAIN_PATH:${TOOLCHAIN_PATH}")
SET(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_LINKER ${TOOLCHAIN_PREFIX}ld${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_OPENOCD openocd${TOOLCHAIN_SUFFIX} CACHE INTERNAL "")
set(CMAKE_C_COMPILER_WORKS 1)  
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

SET(MARCH "rv32imac")
SET(MCPU "ch32v307")
SET(MABI "ilp32") 
SET(MCU_FLAG "-march=${MARCH} -mabi=${MABI}")
SET(COMMON_FLAGS "-Og -g -ffreestanding ")
set(CMAKE_C_FLAGS "${MCU_FLAG} ${COMMON_FLAGS}"  CACHE INTERNAL "c compiler flags")
set(CMAKE_C_FLAGS_DEBUG "-Og -g" CACHE INTERNAL "c compiler flags: Debug")
set(CMAKE_C_FLAGS_RELEASE "-Os -flto" CACHE INTERNAL "c compiler flags: Release")
set(CMAKE_CXX_FLAGS "${MCU_FLAG} ${COMMON_FLAGS} -std=c++11 " CACHE INTERNAL "cxx compiler flags")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g" CACHE INTERNAL "cxx compiler flags: Debug")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -flto" CACHE INTERNAL "cxx compiler flags: Release")
set(CMAKE_ASM_FLAGS  "${MCU_FLAG} ${COMMON_FLAGS}" CACHE INTERNAL "asm compiler flags")
set(CMAKE_ASM_FLAGS_DEBUG "-Og -g" CACHE INTERNAL "asm compiler flags: Debug")
set(CMAKE_ASM_FLAGS_RELEASE "-Os -flto" CACHE INTERNAL "asm compiler flags: Release")
SET(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/firmware/SRC/Ld/Link.ld)
#set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAG} -nostartfiles -Xlinker --gc-sections -std=c99" CACHE INTERNAL "Linker options")
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAG} -nostartfiles -Xlinker --gc-sections -std=c99 --specs=nano.specs --specs=nosys.specs" CACHE INTERNAL "Linker options")

function(generate_library)
    get_filename_component(library_name ${CMAKE_CURRENT_LIST_DIR} NAME)
    message(STATUS "[register library component: ${library_name} ], path:${CMAKE_CURRENT_LIST_DIR}")

    # Add src to lib
    if(ADD_SRCS)
        add_library(${library_name} STATIC ${ADD_SRCS})
        set(include_type PUBLIC)
    else()
        add_library(${library_name} INTERFACE)
        set(include_type INTERFACE)
    endif()

    # Add include
    foreach(include_dir ${ADD_INCLUDE})
        get_filename_component(abs_dir ${include_dir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
        if(NOT IS_DIRECTORY ${abs_dir})
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE}: ${include_dir} not found!")
        endif()
        target_include_directories(${library_name} ${include_type} ${abs_dir})
    endforeach()

    # Add private include
    foreach(include_dir ${ADD_PRIVATE_INCLUDE})
        if(${include_type} STREQUAL INTERFACE)
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE}: ADD_PRIVATE_INCLUDE set but no source file！")
        endif()
        get_filename_component(abs_dir ${include_dir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
        if(NOT IS_DIRECTORY ${abs_dir})
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE}: ${include_dir} not found!")
        endif()
        target_include_directories(${library_name} PRIVATE ${abs_dir})
    endforeach()

    # Add global config include
    target_include_directories(${library_name} PUBLIC ${global_config_dir})

    # Add requirements
    target_link_libraries(${library_name} ${ADD_REQUIREMENTS})

    # Add definitions public
    foreach(difinition ${ADD_DEFINITIONS})
        target_compile_options(${library_name} PUBLIC ${difinition})
    endforeach()

    # Add definitions private
    foreach(difinition ${ADD_DEFINITIONS_PRIVATE})
        target_compile_options(${library_name} PRIVATE ${difinition})
    endforeach()

    # Add static lib
    if(ADD_STATIC_LIB)
        foreach(lib ${ADD_STATIC_LIB})
            if(NOT EXISTS "${lib}")
                prepend(lib_full "${CMAKE_CURRENT_LIST_DIR}/" ${lib})
                if(NOT EXISTS "${lib_full}")
                    message(FATAL_ERROR "Can not find ${lib} or ${lib_full}")
                endif()
                set(lib ${lib_full})
            endif()
            target_link_libraries(${library_name} ${lib})
        endforeach()
    endif()
    # Add dynamic lib
    if(ADD_DYNAMIC_LIB)
        set(dynamic_libs ${g_dynamic_libs})
        foreach(lib ${ADD_DYNAMIC_LIB})
            if(NOT EXISTS "${lib}")
                prepend(lib_full "${CMAKE_CURRENT_LIST_DIR}/" ${lib})
                if(NOT EXISTS "${lib_full}")
                    message(FATAL_ERROR "Can not find ${lib} or ${lib_full}")
                endif()
                set(lib ${lib_full})
            endif()
            list(APPEND dynamic_libs ${lib})
            get_filename_component(lib_dir ${lib} DIRECTORY)
            get_filename_component(lib_name ${lib} NAME)
            target_link_libraries(${library_name} -L${lib_dir} ${lib_name})
        endforeach()
        set(g_dynamic_libs ${dynamic_libs}  CACHE INTERNAL "g_dynamic_libs")
    endif()
endfunction()

function(generate_bin)
    get_filename_component(current_relative_dir_name ${CMAKE_CURRENT_LIST_DIR} NAME)
    
    #上面写法等价于string(REGEX REPLACE ".*/(.*)" "\\1" current_relative_dir_name ${CMAKE_CURRENT_LIST_DIR})
    string(REGEX REPLACE "(.*)/${current_relative_dir_name}$" "\\1" above_absolute_dir ${CMAKE_CURRENT_LIST_DIR})
    
    get_filename_component(above_relative_dir_name ${above_absolute_dir} NAME)
    
    foreach(mainfile IN LISTS mains)
    # Get file name without directory
    get_filename_component(mainname ${mainfile} NAME_WE)
	
    if(DEFINED OUTPUT)
        set(OUTPUT_DIR ${OUTPUT})
        set(target_name firmware)
    else()
        if(${above_relative_dir_name} STREQUAL "firmware")
            set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/cmake_output/${current_relative_dir_name})
            set(target_name ${current_relative_dir_name})
        else()
            set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/cmake_output/${above_relative_dir_name}/${current_relative_dir_name})
            set(target_name ${current_relative_dir_name}_${mainname})
        endif()
    endif()
	
    file(MAKE_DIRECTORY ${OUTPUT_DIR})    
	set(HEX_FILE ${OUTPUT_DIR}/${current_relative_dir_name}.hex)
    set(BIN_FILE ${OUTPUT_DIR}/${current_relative_dir_name}.bin)
    set(MAP_FILE ${OUTPUT_DIR}/${current_relative_dir_name}.map)
    set(DUMP_FILE ${OUTPUT_DIR}/${current_relative_dir_name}.dump)
    
    if(TARGET_REQUIRED_SRCS)
        #list(APPEND SRCS ${TARGET_REQUIRED_SRCS})
        foreach(src ${TARGET_REQUIRED_SRCS})
            if((NOT EXISTS ${src}) AND (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${src}))
            message(FATAL_ERROR "${src} not exist,maybe you should autocomplete your path\r\n")
            endif()
            list(APPEND SRCS ${src})
        endforeach()
    endif()
    
    add_executable(${target_name}.elf ${mainfile} ${SRCS} )
    
    set_target_properties(${target_name}.elf PROPERTIES LINK_FLAGS "-T${LINKER_SCRIPT}")
    set_target_properties(${target_name}.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
    set_target_properties(${target_name}.elf PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
    
    # Add private include
    foreach(include_dir ${TARGET_REQUIRED_PRIVATE_INCLUDE})
        get_filename_component(abs_dir ${include_dir} ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR})
        if(NOT IS_DIRECTORY ${abs_dir})
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE}: ${include_dir} not found!")
        endif()
        target_include_directories(${target_name}.elf PRIVATE ${abs_dir})
    endforeach()
    
    # Add definitions private
    foreach(difinition ${TARGET_REQUIRED_PRIVATE_OPTIONS})
        target_compile_options(${target_name}.elf PRIVATE ${difinition})
    endforeach()   
    
    # Add common libs
    target_link_libraries(${target_name}.elf SRC)
    
    if(TARGET_REQUIRED_LIBS)
        target_link_libraries(${target_name}.elf ${TARGET_REQUIRED_LIBS})       
    endif()
    
    #target_link_libraries(${target_name}.elf "-Wl,-Map=${MAP_FILE}")
    add_custom_command(TARGET ${target_name}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}.elf> ${BIN_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${target_name}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJDUMP} -D $<TARGET_FILE:${target_name}.elf> > ${DUMP_FILE}
        COMMAND ${CMAKE_SIZE} -d $<TARGET_FILE:${target_name}.elf>
        COMMENT "Generate ${BIN_FILE}\r\nGenerate ${HEX_FILE}\r\nGenerate ${DUMP_FILE}\r\nCopy ${BIN_FILE} into download path")
    endforeach()
endfunction(generate_bin)

function(program_flash)
    set(CFG_FILE ${CMAKE_SOURCE_DIR}/firmware/SRC/tools/openocd/wch-riscv.cfg)
    set(HEX_FILE ${CMAKE_SOURCE_DIR}/cmake_output/${APP}/${APP}.hex)
    add_custom_target(programtask
        COMMAND ${CMAKE_OPENOCD} -f ${CFG_FILE} -c init -c halt -c "flash erase_sector wch_riscv 0 last " -c exit
        COMMAND ${CMAKE_OPENOCD} -f ${CFG_FILE} -c init -c halt -c "program ${HEX_FILE} 0x08000000" -c exit
        COMMAND ${CMAKE_OPENOCD} -f ${CFG_FILE} -c init -c halt -c "verify_image ${HEX_FILE}" -c exit
    )
endfunction(program_flash)

function(search_application component_path)
    if(DEFINED APP)
        file(GLOB_RECURSE cmakelists_files ${component_path}/CMakeLists.txt)
        set(app_find_ok 0)
        foreach(cmakelists_file ${cmakelists_files})
        get_filename_component(app_relative_dir ${cmakelists_file} DIRECTORY)
        get_filename_component(app_name ${app_relative_dir} NAME)
            if(${APP} STREQUAL "SRC")
                set(app_find_ok 0)
            # elseif(${APP} STREQUAL "all")
            #     message("[run app:${app_name}],path:${app_relative_dir}")
            #     add_subdirectory(${app_relative_dir})
            #     set(app_find_ok 1)
            elseif(${app_name} MATCHES "^${APP}")
                message("[run app:${app_name}],path:${app_relative_dir}")
                add_subdirectory(${app_relative_dir})
                set(app_find_ok 1)
            endif()
        endforeach()
        if(NOT app_find_ok)
        message(FATAL_ERROR "can not find app:${APP}")
        endif()
    else()
    endif()
endfunction()

PROJECT(OPENCH C CXX ASM)
include_directories(${CMAKE_SOURCE_DIR}/firmware/SRC/Core)
include_directories(${CMAKE_SOURCE_DIR}/firmware/SRC/Debug)
include_directories(${CMAKE_SOURCE_DIR}/firmware/SRC/Peripheral/inc)
add_subdirectory(${CMAKE_SOURCE_DIR}/firmware/SRC)
search_application(${CMAKE_SOURCE_DIR}/firmware)
program_flash()