cmake_minimum_required(VERSION 3.12)

set(index_url_root "https://www.paraview.org/files/dependencies")

set(index_version "5.9.20201204")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "vs2019")
  set(index_subdir "nvidia-index-libs-${index_version}-windows-x64")
  set(sha256sum "a238ebf9d63ca56f5f3253302368731be1f2a58cc65c19275d4c676b59266859")
else ()
  message(FATAL_ERROR
    "Unknown platform for IndeX")
endif ()
set(filename "${index_subdir}.tar.bz2")

# Download the file.
file(DOWNLOAD
  "${index_url_root}/${filename}"
  ".gitlab/${filename}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${sha256sum}")

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

# Move to a predictable prefix.
file(RENAME
  ".gitlab/${index_subdir}"
  ".gitlab/nvidia-index")
