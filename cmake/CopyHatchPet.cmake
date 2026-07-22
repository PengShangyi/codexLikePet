if(NOT DEFINED SOURCE OR NOT IS_DIRECTORY "${SOURCE}")
    message(FATAL_ERROR "Hatch Pet source directory is missing")
endif()
if(NOT DEFINED DESTINATION OR DESTINATION STREQUAL "")
    message(FATAL_ERROR "Hatch Pet bundle destination is missing")
endif()

file(REMOVE_RECURSE "${DESTINATION}")
file(MAKE_DIRECTORY "${DESTINATION}")
file(COPY "${SOURCE}/"
     DESTINATION "${DESTINATION}"
     PATTERN "__pycache__" EXCLUDE
     PATTERN "*.pyc" EXCLUDE
     PATTERN "*.pyo" EXCLUDE)
