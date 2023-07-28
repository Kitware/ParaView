// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqQVTKWidgetEventTranslator_h
#define pqQVTKWidgetEventTranslator_h

#include "pqCoreModule.h"
#include "pqWidgetEventTranslator.h"

/**
Translates low-level Qt events into high-level ParaView events that can be recorded as test cases.

\sa pqEventTranslator
*/

class PQCORE_EXPORT pqQVTKWidgetEventTranslator : public pqWidgetEventTranslator
{
  Q_OBJECT
  typedef pqWidgetEventTranslator Superclass;

public:
  pqQVTKWidgetEventTranslator(QObject* p = nullptr);
  ~pqQVTKWidgetEventTranslator() override;

  using Superclass::translateEvent;
  bool translateEvent(QObject* Object, QEvent* Event, int eventType, bool& Error) override;

private:
  pqQVTKWidgetEventTranslator(const pqQVTKWidgetEventTranslator&);
  pqQVTKWidgetEventTranslator& operator=(const pqQVTKWidgetEventTranslator&);
};

#endif // !pqQVTKWidgetEventTranslator_h
