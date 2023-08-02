// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSaveStateAndScreenshotActions_h
#define pqSaveStateAndScreenshotActions_h

#include <QToolBar>
class pqSaveStateAndScreenshotActions : public QToolBar
{
  Q_OBJECT
public:
  pqSaveStateAndScreenshotActions(QWidget* p);

private:
  Q_DISABLE_COPY(pqSaveStateAndScreenshotActions)
};

#endif
