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
set (Module_SRCS
  pq3DWidget.cxx
  pq3DWidget.h
  pq3DWidgetPropertyWidget.cxx
  pq3DWidgetPropertyWidget.h
  pqBoxWidget.cxx
  pqBoxWidget.h
  pqContourWidget.cxx
  pqContourWidget.h
  pqDistanceWidget.cxx
  pqDistanceWidget.h
  pqHandleWidget.cxx
  pqHandleWidget.h
  pqImplicitPlaneWidget.cxx
  pqImplicitPlaneWidget.h
  pqLineSourceWidget.cxx
  pqLineSourceWidget.h
  pqLineWidget.cxx
  pqLineWidget.h
  pqOrbitWidget.cxx
  pqOrbitWidget.h
  pqPointSourceWidget.cxx
  pqPointSourceWidget.h
  pqPolyLineWidget.cxx
  pqPolyLineWidget.h
  pqSphereWidget.cxx
  pqSphereWidget.h
  pqSplineWidget.cxx
  pqSplineWidget.h
  )

set (Module_MOC_HDRS
  pq3DWidget.h
  pq3DWidgetPropertyWidget.h
  pqBoxWidget.h
  pqContourWidget.h
  pqDistanceWidget.h
  pqHandleWidget.h
  pqImplicitPlaneWidget.h
  pqLineSourceWidget.h
  pqLineWidget.h
  pqOrbitWidget.h
  pqPointSourceWidget.h
  pqPolyLineWidget.h
  pqSphereWidget.h
  pqSplineWidget.h
)

set (Module_UI_FILES
  pqBoxWidget.ui
  pqContourWidget.ui
  pqHandleWidget.ui
  pqImplicitPlaneWidget.ui
  pqLineSourceControls.ui
  pqLineWidget.ui
  pqPointSourceControls.ui
  pqPolyLineWidget.ui
  pqSphereWidget.ui
  pqSplineWidget.ui
)

#-----------------------------------------------------------------------------------
include(ParaViewQt)
pv_find_package_qt(qt_targets REQUIRED QUIET
  QT4_COMPONENTS QtGui
  QT5_COMPONENTS Widgets)
pv_qt_wrap_cpp(__Module_MOC_SRCS ${Module_MOC_HDRS})
pv_qt_wrap_ui(__Module_UI_SRCS ${Module_UI_FILES})
set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module}
  ${Module_SRCS}
  ${__Module_RESOURCE_SRCS}
  ${__Module_MOC_SRCS}
  ${__Module_UI_SRCS})
target_link_libraries(${vtk-module} LINK_PRIVATE ${qt_targets})
