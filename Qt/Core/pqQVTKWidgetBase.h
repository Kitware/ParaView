// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqQVTKWidgetBase_h
#define pqQVTKWidgetBase_h

#if defined(__APPLE__)
// on macOS, we never use QVTKOpenGLStereoWidget, but always the
// QVTKOpenGLNativeWidget. QVTKOpenGLStereoWidget which uses `QWidget::createWindowContainer`
// is not portable and only needed when quad-buffer stereo is being used. Since
// macOS doesn't support quad-buffer stereo, there's no need to use the
// non-portable version.
#include "QVTKOpenGLNativeWidget.h"
using pqQVTKWidgetBase = QVTKOpenGLNativeWidget;
#define PARAVIEW_USING_QVTKOPENGLNATIVEWIDGET 1
#define PARAVIEW_USING_QVTKOPENGLSTEREOWIDGET 0
#else
#include "QVTKOpenGLStereoWidget.h"
using pqQVTKWidgetBase = QVTKOpenGLStereoWidget;
#define PARAVIEW_USING_QVTKOPENGLNATIVEWIDGET 0
#define PARAVIEW_USING_QVTKOPENGLSTEREOWIDGET 1
#endif

#endif
