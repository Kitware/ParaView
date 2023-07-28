// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPVAnimationWidget_h
#define pqPVAnimationWidget_h

#include "pqAnimationViewWidget.h"
#include "pqApplicationComponentsModule.h"

#include "vtkParaViewDeprecation.h"

/**
 * pqPVAnimationWidget is subclass of pqAnimationViewWidget that connects with
 * the pqAnimationManager maintained by pqPVApplicationCore.
 */
class PARAVIEW_DEPRECATED_IN_5_12_0(
  "Use `pqTimeManagerWidget` instead") PQAPPLICATIONCOMPONENTS_EXPORT pqPVAnimationWidget
  : public pqAnimationViewWidget
{
  Q_OBJECT
  typedef pqAnimationViewWidget Superclass;

public:
  pqPVAnimationWidget(QWidget* parent = nullptr);

private:
  Q_DISABLE_COPY(pqPVAnimationWidget)
};

#endif
