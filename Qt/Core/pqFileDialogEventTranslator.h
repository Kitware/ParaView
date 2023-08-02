// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFileDialogEventTranslator_h
#define pqFileDialogEventTranslator_h

#include "pqCoreModule.h"
#include <QPointer>
#include <pqWidgetEventTranslator.h>

class pqFileDialog;

/**
Translates low-level Qt events into high-level ParaView events that can be recorded as test cases.

\sa pqEventTranslator
*/

class PQCORE_EXPORT pqFileDialogEventTranslator : public pqWidgetEventTranslator
{
  Q_OBJECT
  typedef pqWidgetEventTranslator Superclass;

public:
  pqFileDialogEventTranslator(QObject* p = nullptr);

  using Superclass::translateEvent;
  bool translateEvent(QObject* Object, QEvent* Event, bool& Error) override;

private:
  pqFileDialogEventTranslator(const pqFileDialogEventTranslator&);
  pqFileDialogEventTranslator& operator=(const pqFileDialogEventTranslator&);

  QPointer<pqFileDialog> CurrentObject;

private Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void onFilesSelected(const QString&);
  void onCancelled();
};

#endif // !pqFileDialogEventTranslator_h
