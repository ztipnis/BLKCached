# first we can indicate the documentation build as an option and set it to ON by default
option(BUILD_DOC "Build documentation" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# check if Doxygen is installed
find_package(Doxygen)
find_package(Sphinx)
if (DOXYGEN_FOUND AND SPHINX_FOUND)

    #This will be the main output of our command
    set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/src)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen)
    set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
    set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
    set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/docs/sphinx)
     
    #Replace variables inside @@ with the current values
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)
     
    file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}) #Doxygen won't create this for us
    add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                       DEPENDS ${CAT_CUTIFIER_PUBLIC_HEADERS}
                       COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                       MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                       COMMENT "Generating docs")
    add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})
    set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
    set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
    set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)
     
    # Only regenerate Sphinx when:
    # - Doxygen has rerun
    # - Our doc files have been updated
    # - The Sphinx config has been updated
    add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
                       COMMAND 
                         ${SPHINX_EXECUTABLE} -Mjekyll
                         -Dbreathe_projects.BLKCached=${DOXYGEN_OUTPUT_DIR}/xml
                       ${SPHINX_SOURCE} ${SPHINX_BUILD}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       DEPENDS
                       # Other docs files you want to track should go here (or in some variable)
                       ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
                       ${DOXYGEN_INDEX_FILE}
                       MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
                       COMMENT "Generating documentation with Sphinx")
     
    # Nice named target so we can run the job easily
    add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})
     
    # Add an install target to install the docs
    include(GNUInstallDirs)
    install(DIRECTORY ${SPHINX_BUILD}
    DESTINATION ${CMAKE_INSTALL_DOCDIR})
else (DOXYGEN_FOUND AND SPHINX_FOUND)
    if(NOT DOXYGEN_FOUND)
        message("Doxygen not found")
    endif()
    if(NOT SPHINX_FOUND)
        message("Sphinx not found")
    endif()
  message("Doxygen and Sphinx need to be installed to generate documentation")
endif (DOXYGEN_FOUND AND SPHINX_FOUND)
