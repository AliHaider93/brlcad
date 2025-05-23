if(NOT DEFINED BRLCAD_BUILDTEST_EXTERNALS)
  # Minimum required version of CMake
  cmake_minimum_required(VERSION 3.20)

  # Set CMake project name
  project(G2SAT)
else(NOT DEFINED BRLCAD_BUILDTEST_EXTERNALS)
  set(
    SAT_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}/../../../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../include"
    "${OPENNURBS_INCLUDE_DIRS}"
    ${ZLIB_INCLUDE_DIRS}
    ${REGEX_INCLUDE_DIRS}
  )

  # The build shims aren't so great at mocking the initialization
  # behavior of the real code.
  check_cxx_flag("Wno-maybe-uninitialized")

  set(g_sat_srcs g-sat.cpp)
  add_definitions(-DTEST_BUILD)
  add_library(g-sat ${g_sat_srcs})
  target_include_directories(g-sat BEFORE PRIVATE ${SAT_DIRS})
  target_link_libraries(
    g-sat
    librt
    libwdb
    libbrep
    libbg
    libnmg
    libbv
    libbn
    libbu
  )
  set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS BRLCADBUILD HAVE_CONFIG_H)
  set_target_properties(g-sat PROPERTIES FOLDER "BRL-CAD Executables/Build Only")
  if(HIDE_INTERNAL_SYMBOLS)
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "BU_DLL_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "BN_DLL_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "BV_DLL_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "NMG_DLL_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "BG_DLL_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "BREP_DLL_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "RT_DLL_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "DB5_DLL_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "WDB_DLL_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "TIE_DLL_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "OPENNURBS_IMPORTS")
    set_property(TARGET g-sat APPEND PROPERTY COMPILE_DEFINITIONS "TINYCTHREAD_DLL_IMPORTS")
  endif(HIDE_INTERNAL_SYMBOLS)

  cmakefiles(${g_sat_srcs} shim.h)
  cmakefiles(CMakeLists.txt)
endif(NOT DEFINED BRLCAD_BUILDTEST_EXTERNALS)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
