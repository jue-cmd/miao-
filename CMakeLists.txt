cmake_minimum_required(VERSION 3.20)

# Must be set before project()/enable_language(ASM_NASM).
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS_INIT "-I${CMAKE_CURRENT_LIST_DIR}/include/")

project(SimpleOS
    VERSION 0.1.0
    DESCRIPTION "A simple educational operating system"
    LANGUAGES C ASM_NASM
)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(OS_WARNINGS "Enable extra compiler warnings" ON)
option(OS_WERROR "Treat warnings as errors" OFF)
option(OS_BUILD_LIST "Build kernel/list.c (WIP; off by default)" OFF)

set(OS_DISK_IMAGE "${CMAKE_SOURCE_DIR}/c.img" CACHE FILEPATH "Raw disk image written by the build")
set(OS_DISK_SIZE_MIB 32 CACHE STRING "Disk image size in MiB when creating a new image")
set(OS_KERNEL_LOAD_ADDR 0xc0001500 CACHE STRING "Kernel link / entry virtual address")
set(OS_KERNEL_SEEK 9 CACHE STRING "Kernel start LBA on disk")
set(OS_KERNEL_SECTORS 200 CACHE STRING "Max sectors written for the kernel")
set(OS_LOADER_SEEK 2 CACHE STRING "Loader start LBA on disk")

# ---------------------------------------------------------------------------
# Host tools
# ---------------------------------------------------------------------------
find_program(NASM_EXECUTABLE nasm REQUIRED)
find_program(DD_EXECUTABLE dd REQUIRED)
find_program(QEMU_EXECUTABLE qemu-system-i386)
find_program(BOCHS_EXECUTABLE bochs)

message(STATUS "NASM  : ${NASM_EXECUTABLE}")
message(STATUS "Linker: ${CMAKE_LINKER}")
message(STATUS "Disk  : ${OS_DISK_IMAGE}")

# ---------------------------------------------------------------------------
# Language / include paths
# ---------------------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# gnu11: need GNU asm keyword and freestanding kernel C.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Keep NASM include path available for incremental reconfigure.
set(CMAKE_ASM_NASM_FLAGS "-I${CMAKE_SOURCE_DIR}/include/ ${CMAKE_ASM_NASM_FLAGS}")

# ---------------------------------------------------------------------------
# Kernel compile flags (i386 freestanding)
# ---------------------------------------------------------------------------
set(KERNEL_C_FLAGS
    -m32
    -ffreestanding
    -fno-builtin
    -fno-pic
    -fno-pie
    -fno-stack-protector
    -fno-omit-frame-pointer
)

if(OS_WARNINGS)
    list(APPEND KERNEL_C_FLAGS -Wall -Wextra)
endif()
if(OS_WERROR)
    list(APPEND KERNEL_C_FLAGS -Werror)
endif()

# ---------------------------------------------------------------------------
# Kernel object libraries (explicit link order, matching sh/make_kernel.sh)
# ---------------------------------------------------------------------------
function(os_kernel_c_object name source)
    add_library(${name} OBJECT ${source})
    target_compile_options(${name} PRIVATE ${KERNEL_C_FLAGS})
    target_include_directories(${name} PRIVATE
        "${CMAKE_SOURCE_DIR}/kernel"
    )
endfunction()

os_kernel_c_object(kernel_main      kernel/main.c)
os_kernel_c_object(kernel_strings   kernel/strings.c)
os_kernel_c_object(kernel_interrupt kernel/interrupt.c)
os_kernel_c_object(kernel_init      kernel/init.c)
os_kernel_c_object(kernel_debug     kernel/debug.c)
os_kernel_c_object(kernel_bitmap    kernel/bitmap.c)
os_kernel_c_object(kernel_memory    kernel/memory.c)
os_kernel_c_object(kernel_list      kernel/list.c)
os_kernel_c_object(kernel_timer     kernel/device/timer.c)
os_kernel_c_object(kernel_thread    kernel/thread/thread.c)

add_library(kernel_asm OBJECT kernel/kernel.asm)
add_library(kernel_print_asm OBJECT kernel/print.asm)
add_library(kernel_switch_asm OBJECT kernel/thread/switch_to.asm)

set(KERNEL_LINK_OBJECTS
    $<TARGET_OBJECTS:kernel_main>
    $<TARGET_OBJECTS:kernel_print_asm>
    $<TARGET_OBJECTS:kernel_strings>
    $<TARGET_OBJECTS:kernel_interrupt>
    $<TARGET_OBJECTS:kernel_init>
    $<TARGET_OBJECTS:kernel_asm>
    $<TARGET_OBJECTS:kernel_timer>
    $<TARGET_OBJECTS:kernel_debug>
    $<TARGET_OBJECTS:kernel_bitmap>
    $<TARGET_OBJECTS:kernel_memory>
    $<TARGET_OBJECTS:kernel_list>
    $<TARGET_OBJECTS:kernel_thread>
    $<TARGET_OBJECTS:kernel_switch_asm>
)

set(KERNEL_OBJECT_TARGETS
    kernel_main kernel_print_asm kernel_strings kernel_interrupt
    kernel_init kernel_asm kernel_timer kernel_debug kernel_bitmap
    kernel_memory kernel_list kernel_thread kernel_switch_asm
)

if(OS_BUILD_LIST)
    # list.c 已默认编入
endif()

set(KERNEL_BIN "${CMAKE_BINARY_DIR}/kernel.bin")

# 用 custom_target 链接：始终在依赖的 .o 更新后重链，避免 OBJECT 库
# 只 DEPENDS 目标名时出现「编了 .o 却不更新 kernel.bin」的问题。
add_custom_target(kernel ALL
    COMMAND ${CMAKE_LINKER} -m elf_i386
            -Ttext ${OS_KERNEL_LOAD_ADDR}
            -e main
            -o "${KERNEL_BIN}"
            ${KERNEL_LINK_OBJECTS}
    DEPENDS ${KERNEL_OBJECT_TARGETS}
    COMMAND_EXPAND_LISTS
    BYPRODUCTS "${KERNEL_BIN}"
    COMMENT "Linking kernel.bin @ ${OS_KERNEL_LOAD_ADDR}"
    VERBATIM
)

# ---------------------------------------------------------------------------
# Boot: MBR + loader (flat binaries)
# ---------------------------------------------------------------------------
set(MBR_BIN "${CMAKE_BINARY_DIR}/mbr.bin")
set(LOADER_BIN "${CMAKE_BINARY_DIR}/loader.bin")

add_custom_command(
    OUTPUT "${MBR_BIN}"
    COMMAND ${NASM_EXECUTABLE}
            -I "${CMAKE_SOURCE_DIR}/"
            -I "${CMAKE_SOURCE_DIR}/include/"
            -f bin
            -o "${MBR_BIN}"
            "${CMAKE_SOURCE_DIR}/mbr/mbr.asm"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/mbr/mbr.asm"
        "${CMAKE_SOURCE_DIR}/include/boot.inc"
    COMMENT "Assembling MBR"
    VERBATIM
)

add_custom_command(
    OUTPUT "${LOADER_BIN}"
    COMMAND ${NASM_EXECUTABLE}
            -I "${CMAKE_SOURCE_DIR}/include/"
            -f bin
            -o "${LOADER_BIN}"
            "${CMAKE_SOURCE_DIR}/mbr/loader.asm"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/mbr/loader.asm"
        "${CMAKE_SOURCE_DIR}/include/boot.inc"
        "${CMAKE_SOURCE_DIR}/include/elf.inc"
    COMMENT "Assembling loader"
    VERBATIM
)

add_custom_target(boot ALL DEPENDS "${MBR_BIN}" "${LOADER_BIN}")

# ---------------------------------------------------------------------------
# Disk image
# ---------------------------------------------------------------------------
add_custom_target(image ALL
    COMMAND ${CMAKE_COMMAND}
            -DDISK_IMAGE=${OS_DISK_IMAGE}
            -DDISK_SIZE_MIB=${OS_DISK_SIZE_MIB}
            -DMBR_BIN=${MBR_BIN}
            -DLOADER_BIN=${LOADER_BIN}
            -DKERNEL_BIN=${KERNEL_BIN}
            -DLOADER_SEEK=${OS_LOADER_SEEK}
            -DKERNEL_SEEK=${OS_KERNEL_SEEK}
            -DKERNEL_SECTORS=${OS_KERNEL_SECTORS}
            -DDD_EXECUTABLE=${DD_EXECUTABLE}
            -P "${CMAKE_SOURCE_DIR}/cmake/write_disk_image.cmake"
    DEPENDS boot kernel
    COMMENT "Writing MBR/loader/kernel into ${OS_DISK_IMAGE}"
    VERBATIM
)

# ---------------------------------------------------------------------------
# Run / debug helpers
# ---------------------------------------------------------------------------
if(QEMU_EXECUTABLE)
    add_custom_target(run
        COMMAND ${QEMU_EXECUTABLE}
                -m 256
                -vga std
                -display sdl
                -drive file=${OS_DISK_IMAGE},format=raw,media=disk
                -boot order=c
                -serial stdio
        DEPENDS image
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        USES_TERMINAL
        COMMENT "Booting ${OS_DISK_IMAGE} in QEMU"
    )

    add_custom_target(run-gdb
        COMMAND ${QEMU_EXECUTABLE}
                -m 256
                -vga std
                -display sdl
                -drive file=${OS_DISK_IMAGE},format=raw,media=disk
                -boot order=c
                -serial stdio
                -s -S
        DEPENDS image
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        USES_TERMINAL
        COMMENT "QEMU waiting for GDB on :1234 (-s -S)"
    )
endif()

if(BOCHS_EXECUTABLE)
    add_custom_target(bochs
        COMMAND ${BOCHS_EXECUTABLE} -f "${CMAKE_SOURCE_DIR}/bochsrc.disk" -q
        DEPENDS image
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        USES_TERMINAL
        COMMENT "Booting with Bochs"
    )
endif()

# ---------------------------------------------------------------------------
# Install / summary
# ---------------------------------------------------------------------------
install(FILES
    "${KERNEL_BIN}"
    "${MBR_BIN}"
    "${LOADER_BIN}"
    DESTINATION bin
)

message(STATUS "")
message(STATUS "SimpleOS ${PROJECT_VERSION} configured")
message(STATUS "  Build tree : ${CMAKE_BINARY_DIR}")
message(STATUS "  Targets    : boot, kernel, image, run, run-gdb, bochs")
message(STATUS "  Configure  : cmake -B build -S .")
message(STATUS "  Build      : cmake --build build")
message(STATUS "  Run        : cmake --build build --target run")
message(STATUS "")
