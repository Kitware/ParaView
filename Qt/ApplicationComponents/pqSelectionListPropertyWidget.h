// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSelectionListPropertyWidget_h
#define pqSelectionListPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyGroupWidget.h"

#include <QScopedPointer>
#include <QVariant>

#include "pqSMProxy.h" // For property.

class vtkSMProxy;
class vtkSMPropertyGroup;

/**
 * pqSelectionListPropertyWidget is a custom widget used to associate a label for each selectionNode
 * from an input selection.
 *
 * It also use the pqSelectionInputWidget to know the number of labels we need to define, by copying
 * the active selection.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSelectionListPropertyWidget : public pqPropertyGroupWidget
{
  Q_OBJECT
  typedef pqPropertyGroupWidget Superclass;
  Q_PROPERTY(QList<QVariant> labels READ labels WRITE setLabels NOTIFY labelsChanged);

public:
  pqSelectionListPropertyWidget(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject = nullptr);
  ~pqSelectionListPropertyWidget() override;

  /**
   * Methods used to set/get the label values for the widget.
   *
   * The number of labels required depend of the number of SelectionNode in the activeSelection
   * provided by the pqSelectionInputWidget.
   */
  QList<QVariant> labels() const;
  void setLabels(const QList<QVariant>& labels);

Q_SIGNALS:
  /**
   * Signal for the labels proxy changed.
   */
  void labelsChanged();

protected Q_SLOTS:
  /**
   * Clone the active selection handled by the selection manager.
   */
  void populateRowLabels(pqSMProxy appendSelection);

private:
  Q_DISABLE_COPY(pqSelectionListPropertyWidget)

  class pqUi;
  QScopedPointer<pqUi> Ui;
};

#endif
