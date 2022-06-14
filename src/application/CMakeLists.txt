# Zeus GPMCU - Application

#print helper
function(list_print liItems)
    message(STATUS "GPMCU_APP_SOURCES will contain: ")
  foreach(f ${${liItems}})
    message(STATUS ${f})
  endforeach()
endfunction()

macro( add_recursive dir retVal)
    # Store unfiltered list of files into temporary list
    file( GLOB_RECURSE _tmp_list ${dir}/*.c)
    # Resulted list is initially empty. Copy all needed elements into it.
    set(${retval} "")
    foreach(f ${_tmp_list})
        if(NOT f MATCHES ".unit_test.") # Remove unit tests
            list(APPEND ${retVal} ${f})
        endif()
    endforeach()

endmacro()

add_recursive(${CMAKE_CURRENT_LIST_DIR} SOURCES)
list(APPEND SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/../bootloader/bootloader_helpers.c
  ${CMAKE_CURRENT_LIST_DIR}/../bootloader/bootloader_usb_helpers.c
  )
list_print(SOURCES)
set(GPMCU_APP_SOURCES ${SOURCES} PARENT_SCOPE)

