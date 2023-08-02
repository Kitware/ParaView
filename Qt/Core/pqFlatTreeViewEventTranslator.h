// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFlatTreeViewEventTranslator_h
#define pqFlatTreeViewEventTranslator_h

#include "pqCoreModule.h"
#include "pqWidgetEventTranslator.h"
#include <QPoint>

/**
Translates low-level Qt events into high-level ParaView events that can be recorded as test cases.

\sa pqEventTranslator
*/

class PQCORE_EXPORT pqFlatTreeViewEventTranslator : public pqWidgetEventTranslator
{
  Q_OBJECT
  typedef pqWidgetEventTranslator Superclass;

public:
  pqFlatTreeViewEventTranslator(QObject* p = nullptr);

  using Superclass::translateEvent;
  bool translateEvent(QObject* Object, QEvent* Event, bool& Error) override;

protected:
  QPoint LastPos;

private:
  pqFlatTreeViewEventTranslator(const pqFlatTreeViewEventTranslator&);
  pqFlatTreeViewEventTranslator& operator=(const pqFlatTreeViewEventTranslator&);
};

#endif // !pqFlatTreeViewEventTranslator_h
