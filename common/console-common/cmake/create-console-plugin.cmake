macro(create_console_plugin LIBRARY_NAME)
  SET(COMPLETE_LIBRARY_PATH "${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_LIB_DESTINATION}/lib${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")
  SET(PLUGIN_LIST_FILE_PATH "${CATKIN_DEVEL_PREFIX}/${CATKIN_GLOBAL_SHARE_DESTINATION}/console-plugins.txt")
  message(STATUS "Adding library \"${COMPLETE_LIBRARY_PATH}\" to list in \"${PLUGIN_LIST_FILE_PATH}\".")
  FILE(APPEND ${PLUGIN_LIST_FILE_PATH} "${COMPLETE_LIBRARY_PATH}\n")
endmacro()