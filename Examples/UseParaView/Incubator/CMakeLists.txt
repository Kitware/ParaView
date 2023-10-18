cmake_minimum_required(VERSION 3.12)
project(Incubator)

find_package(ParaView COMPONENTS INCUBATOR)
if (NOT ParaView_FOUND)
  message(FATAL_ERROR
    "Failed to find ParaView: ${ParaView_NOT_FOUND_MESSAGE}")
endif ()
if (NOT DEFINED ParaView_INCUBATOR_FOUND)
  message(FATAL_ERROR
    "ParaView failed to set the `INCUBATOR` component flag")
endif ()
if (NOT ParaView_INCUBATOR_FOUND)
  message(FATAL_ERROR
    "ParaView failed to find the `INCUBATOR` component flag: ${ParaView_INCUBATOR_NOT_FOUND_MESSAGE}")
endif ()
