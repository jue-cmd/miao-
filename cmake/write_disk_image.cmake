# Write boot artifacts into a flat disk image (invoked from CMakeLists.txt).
if(NOT EXISTS "${DISK_IMAGE}")
  message(STATUS "Creating new disk image (${DISK_SIZE_MIB} MiB): ${DISK_IMAGE}")
  execute_process(
    COMMAND ${DD_EXECUTABLE} if=/dev/zero of=${DISK_IMAGE} bs=1M count=${DISK_SIZE_MIB} status=none
    RESULT_VARIABLE dd_create_rc
  )
  if(NOT dd_create_rc EQUAL 0)
    message(FATAL_ERROR "Failed to create disk image ${DISK_IMAGE}")
  endif()
endif()

macro(os_dd_write infile seek count)
  if("${count}" STREQUAL "")
    execute_process(
      COMMAND ${DD_EXECUTABLE} if=${infile} of=${DISK_IMAGE}
              bs=512 seek=${seek} conv=notrunc status=none
      RESULT_VARIABLE _dd_rc
    )
  else()
    execute_process(
      COMMAND ${DD_EXECUTABLE} if=${infile} of=${DISK_IMAGE}
              bs=512 count=${count} seek=${seek} conv=notrunc status=none
      RESULT_VARIABLE _dd_rc
    )
  endif()
  if(NOT _dd_rc EQUAL 0)
    message(FATAL_ERROR "dd failed writing ${infile} -> ${DISK_IMAGE} (seek=${seek})")
  endif()
endmacro()

message(STATUS "Writing MBR    -> LBA 0")
os_dd_write("${MBR_BIN}" 0 "")

message(STATUS "Writing loader -> LBA ${LOADER_SEEK}")
os_dd_write("${LOADER_BIN}" "${LOADER_SEEK}" "")

message(STATUS "Writing kernel -> LBA ${KERNEL_SEEK} (max ${KERNEL_SECTORS} sectors)")
os_dd_write("${KERNEL_BIN}" "${KERNEL_SEEK}" "${KERNEL_SECTORS}")

message(STATUS "Disk image updated: ${DISK_IMAGE}")
