// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpreadSheetVisibilityBehavior_h
#define pqSpreadSheetVisibilityBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class pqView;

/**
 * @ingroup Behaviors
 * Whenever spreadsheet view is created, ParaView wants to ensure that the
 * active source is automatically displayed in that view. This is managed by
 * this behavior.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSpreadSheetVisibilityBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqSpreadSheetVisibilityBehavior(QObject* parent = nullptr);

protected Q_SLOTS:
  void showActiveSource(pqView*);

private:
  Q_DISABLE_COPY(pqSpreadSheetVisibilityBehavior)
};

#endif
