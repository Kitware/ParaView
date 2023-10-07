cmake_minimum_required(VERSION 3.12)
project(NoIncubator)

find_package(ParaView)
if (NOT ParaView_FOUND)
  message(FATAL_ERROR
    "Failed to find ParaView: ${ParaView_NOT_FOUND_MESSAGE}")
endif ()
if (NOT DEFINED ParaView_INCUBATOR_FOUND)
  message(FATAL_ERROR
    "ParaView failed to set the `INCUBATOR` component flag")
endif ()
if (ParaView_INCUBATOR_FOUND)
  message(FATAL_ERROR
    "ParaView found the `INCUBATOR` component without requesting it")
endif ()
if (NOT DEFINED ParaView_INCUBATOR_NOT_FOUND_MESSAGE)
  message(FATAL_ERROR
    "ParaView did not set the `ParaView_INCUBATOR_NOT_FOUND_MESSAGE` variable")
endif ()
