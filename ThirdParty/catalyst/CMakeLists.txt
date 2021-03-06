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
vtk_module_third_party_internal(
  VERSION "master"
  LICENSE_FILES "vtkcatalyst/catalyst/License.txt"
  SUBDIRECTORY vtkcatalyst
  INTERFACE)

# Store the variable we need for Catalyst implementations inside of ParaView.
# This is needed because the `find_package(catalyst)` isn't at the right scope
# for its usage in ParaView's minimum CMake version. This (and usages of
# `paraview_catalyst_macro_directory`) can be removed once CMake 3.17 is
# required.
if (VTK_MODULE_USE_EXTERNAL_ParaView_catalyst)
  set_property(GLOBAL PROPERTY
      paraview_catalyst_macro_directory "${_catalyst_macro_directory}")
else ()
  set_property(GLOBAL PROPERTY
      paraview_catalyst_macro_directory "${CMAKE_CURRENT_SOURCE_DIR}/vtkcatalyst/catalyst/cmake")
endif ()
