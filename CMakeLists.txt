CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

# Add all subdirectories containing CMakeLists

SUBDIRLIST(LOWPOWER_LIB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
FOREACH(subdir ${LOWPOWER_LIB_DIRS})
	# ignore git index directory
	IF(NOT "${subdir}" STREQUAL ".git")
		IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt")
			MESSAGE(STATUS "Configuring low-power module: ${subdir}")
			ADD_SUBDIRECTORY(${subdir})
		ELSE()
			MESSAGE(WARNING "Low-power module '${subdir}' does not contain CMakeLists.txt file")
		ENDIF()
	ENDIF()
ENDFOREACH()
