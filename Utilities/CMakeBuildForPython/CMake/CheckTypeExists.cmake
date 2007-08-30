# - Check if the given struct or class has the specified member variable
# CHECK_STRUCT_MEMBER (STRUCT MEMBER HEADER VARIABLE)
#
#  STRUCT - the name of the struct or class you are interested in
#  MEMBER - the member which existence you want to check
#  HEADER - the header(s) where the prototype should be declared
#  VARIABLE - variable to store the result
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


INCLUDE(CheckTypeSize)

MACRO (CHECK_TYPE_EXISTS _TYPE _HEADER _RESULT)
   SET(CMAKE_EXTRA_INCLUDE_FILES ${_HEADER})
   CHECK_TYPE_SIZE(${_TYPE} ${_RESULT})
ENDMACRO (CHECK_TYPE_EXISTS)

