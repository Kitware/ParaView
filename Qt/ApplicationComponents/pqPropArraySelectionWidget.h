// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPropArraySelectionWidget_h
#define pqPropArraySelectionWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

/**
 * Special widget to be used in exporter panels as a PropertyGroup widget, that allow the selection
 * of exported arrays for each source mapped to an actor in the view. Displays a combobox to select
 * the edited source/prop, and point & cell selection tables for each of those.
 *
 * The PropertyGroup that we attach this widget to must have 3 properties: prop (string for
 * selection of edited source), point_arrays and cell_arrays.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPropArraySelectionWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqPropArraySelectionWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqPropArraySelectionWidget() override;

Q_SIGNALS:
  void widgetModified();

private Q_SLOTS:
  /**
   * When point domain changed,
   */
  void pointDomainChanged();

  /**
   *
   */
  void cellDomainChanged();

private:
  Q_DISABLE_COPY(pqPropArraySelectionWidget);

  /**
   * emits widgetModified event
   */
  void updateProperties();

  /**
   * Memorize current values of checked lists so we can put them back in the same state later.
   */
  void updateInternalMemory();

  class pqInternals;
  pqInternals* Internals;
};

#endif
