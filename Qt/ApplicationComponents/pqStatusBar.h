// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqStatusBar_h
#define pqStatusBar_h

#include <QScopedPointer>
#include <QStatusBar>

#include "vtkNew.h"

#include "pqApplicationComponentsModule.h"

class vtkPVSystemConfigInformation;
class QToolButton;
class QProgressBar;
class QStyle;

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

Q_SIGNALS: // NOLINT(readability-redundant-access-specifiers)
  void messageIndicatorPressed();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers
  void handleMessage(const QString& message, int type);
  void resetMessageIndicators();
protected Q_SLOTS:
  void updateServerConfigInfo();
  void updateMemoryProgressBar();

protected: // NOLINT(readability-redundant-access-specifiers)
  vtkNew<vtkPVSystemConfigInformation> ServerConfigsInfo;
  QProgressBar* MemoryProgressBar;
  QToolButton* ErrorIndicator;
  std::uint64_t ErrorCount = 0;
  QToolButton* WarningIndicator;
  std::uint64_t WarningCount = 0;

private:
  Q_DISABLE_COPY(pqStatusBar)

  void updateWarningIndicator();
  void updateErrorIndicator();

  QScopedPointer<QStyle> ProgressBarStyle;
};

#endif
