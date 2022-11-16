cmake_minimum_required(VERSION 3.12)

# Input variables.
set(cdi_version "2.1.0")
set(cdi_build_date "20230213.1")

set(cdi_url "https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/cdi/v${cdi_version}-${cdi_build_date}")

set(filename)
set(checksum)
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(filename "cdi-macos11.0-aarch64.tar.gz")
  set(checksum "0707fe6a1f6b2aada40e2f855a1a1496e97565451249f344dcc564ebcd9d35a652120f94991fdf9e2aebf1a0ee6ef40520d714e4a1146be1ae8c4d7862474995")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  set(filename "cdi-macos10.13-x86_64.tar.gz")
  set(checksum "61cd9c34ae3eeaee7cb1d459202821383efb19556ec33cf207dcca6376bdaf5fa9b19e1d6e7e8d6d3482990f783dbd05e719932e9781be306c7e58848a92b92f")
else ()
  message(FATAL_ERROR
    "Unknown ABI to use for cdi")
endif ()

message(WARNING "${cdi_url}/${filename}")

# Download the file.
file(DOWNLOAD
  "${cdi_url}/${filename}"
  ".gitlab/${filename}"
  STATUS download_status
  EXPECTED_HASH "SHA512=${checksum}")

# Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${filename}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
  "${CMAKE_COMMAND}"
  -E tar
  xf "${filename}"
  WORKING_DIRECTORY ".gitlab"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${filename}: ${err}")
endif ()
