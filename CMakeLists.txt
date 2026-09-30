# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# Excluded: still empty stub, not yet wired in. See HANDOFF.md.
set(EXCLUDE_COMPONENTS "wifi_credentials")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lvgl_test)
