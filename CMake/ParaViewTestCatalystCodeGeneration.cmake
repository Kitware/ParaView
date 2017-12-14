#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================

#
# This script tests generating Catalyst sources from the editions. This
# will only test on Unix systems.
#
# The script expects the following input variables:
# PYTHON_EXECUTABLE : The Python executable to catalyze the source code
# PARAVIEW_SOURCE_DIR : ParaView source dir (For source location of the Editions
# PARAVIEW_TEST_DIR : Temporary directory for location of Catalyst generated source code

message (STATUS "Testing building Catalyst editions")

execute_process (COMMAND ${PYTHON_EXECUTABLE}
  ${PARAVIEW_SOURCE_DIR}/Catalyst/catalyze.py
  -r ${PARAVIEW_SOURCE_DIR}
  -i Editions/Base -i Editions/Essentials -i Editions/Enable-Python/
  -i Editions/Extras -i Editions/Rendering-Base -i Editions/Rendering-Base-Python
  -o ${PARAVIEW_TEST_DIR}/CatalystEditions
  WORKING_DIRECTORY ${PARAVIEW_SOURCE_DIR}/Catalyst
  RESULT_VARIABLE irv)
if (NOT irv EQUAL 0)
  message(FATAL_ERROR "Could not generate Catalyst source code")
endif ()

# cmake.sh is one of the last things that is generated when processing
# Catalyst editions so check to see if that file exists for a bit of verification.
if (NOT EXISTS ${PARAVIEW_TEST_DIR}/CatalystEditions/cmake.sh)
  message(FATAL_ERROR "Could not find Catalyst cmake.sh generated file")
endif ()
