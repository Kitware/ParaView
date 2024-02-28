// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqStatusBar_h
#define pqStatusBar_h

#include <QStatusBar>

#include "vtkNew.h"

#include "pqApplicationComponentsModule.h"

class vtkPVSystemConfigInformation;
class QProgressBar;

/**
 * pqStatusBar extends QStatusBar to support showing paraview progress.
 * It uses pqProgressManager provided by pqApplicationCore to show the
 * progress values. Internally uses pqProgressWidget to show the progress.
 * It also shows a memory status bar that automatically updates.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqStatusBar : public QStatusBar
{
  Q_OBJECT
  typedef QStatusBar Superclass;

public:
  pqStatusBar(QWidget* parent = nullptr);
  ~pqStatusBar() override;

protected Q_SLOTS:
  void updateServerConfigInfo();
  void updateMemoryProgressBar();

protected: // NOLINT(readability-redundant-access-specifiers)
  vtkNew<vtkPVSystemConfigInformation> ServerConfigsInfo;
  QProgressBar* MemoryProgressBar;

private:
  Q_DISABLE_COPY(pqStatusBar)
};

#endif
