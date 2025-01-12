function(copy_resource_file RESOURCE_FILE DESTINATION)
    if(NOT EXISTS ${CMAKE_SOURCE_DIR}/resources/${RESOURCE_FILE})
        message(FATAL_ERROR "Resource file ${RESOURCE_FILE} does not exist!")
    endif()

    add_custom_command(
        OUTPUT ${DESTINATION}/${RESOURCE_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/resources/${RESOURCE_FILE} ${DESTINATION}
        DEPENDS ${CMAKE_SOURCE_DIR}/resources/${RESOURCE_FILE}
        COMMENT "Copying ${CMAKE_SOURCE_DIR}/resources/${RESOURCE_FILE} to ${DESTINATION}"
    )

    add_custom_target(copy-${RESOURCE_FILE}-target ALL
        DEPENDS ${DESTINATION}/${RESOURCE_FILE}
    )
endfunction()