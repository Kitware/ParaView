// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLightToolbar_h
#define pqLightToolbar_h

#include "pqApplicationComponentsModule.h"
#include <QToolBar>

#include <memory> // for unique_ptr

class pqView;

/**
 * pqLightToolbar is the toolbar that has buttons for controlling light settings.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLightToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqLightToolbar(const QString& title, QWidget* parentObject = nullptr);
  pqLightToolbar(QWidget* parentObject = nullptr);
  ~pqLightToolbar() override;

protected Q_SLOTS:
  void setView(pqView* view);
  void toggleLightKit();

private:
  Q_DISABLE_COPY(pqLightToolbar)

  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
  void constructor();
};

#endif
