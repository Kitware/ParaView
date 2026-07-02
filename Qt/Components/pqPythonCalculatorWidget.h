// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonCalculatorWidget_h
#define pqPythonCalculatorWidget_h

#include "pqComponentsModule.h" // For PQCOMPONENTS_EXPORT.
#include "pqStringVectorPropertyWidget.h"

#include <QScopedPointer> // For QScopedPointer
#include <QString>        // For QString

class pqTreeWidget;
class QTreeWidgetItem;

/**
 * pqPythonCalculatorWidget is a property-widget that shows a Python calculator
 * expression editor with input/array pickers.
 */
class PQCOMPONENTS_EXPORT pqPythonCalculatorWidget : public pqStringVectorPropertyWidget
{
  Q_OBJECT
  typedef pqStringVectorPropertyWidget Superclass;

public:
  pqPythonCalculatorWidget(vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqPythonCalculatorWidget() override;

protected Q_SLOTS:
  void updateInputs();
  void updateArrays();
  void arrayChosen(int index);
  void functionActivated(QTreeWidgetItem* item, int column);
  void filterFunctions(const QString& text);

private:
  Q_DISABLE_COPY(pqPythonCalculatorWidget)

  QString escapeArrayName(const QString& name) const;
  int currentInputIndex() const;

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
