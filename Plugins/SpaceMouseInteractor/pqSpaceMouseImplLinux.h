// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpaceMouseImplLinux_h
#define pqSpaceMouseImplLinux_h
#include "vtkCamera.h"
#include "vtkWeakPointer.h"

#include <QObject>
class pqView;

/// Placeholder for non-windows platforms
class pqSpaceMouseImpl : public QObject
{
  Q_OBJECT

public:
  pqSpaceMouseImpl();
  ~pqSpaceMouseImpl() override;

public Q_SLOTS:
  /// which view are we controlling? The active one.
  void setActiveView(pqView* view);
  /// the active camera changed.
  void cameraChanged();

public:
  void render();

protected:
  vtkWeakPointer<vtkCamera> Camera;
};

#endif
