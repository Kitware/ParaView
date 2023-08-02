// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSelectionQueryPropertyWidget_h
#define pqSelectionQueryPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

#include <QScopedPointer> // for QScopedPointer.
#include <QStringList>

/**
 * @class pqSelectionQueryPropertyWidget
 * @brief property-widget for creating selection queries.
 *
 * pqSelectionQueryPropertyWidget is a pqPropertyWidget subclass that can be
 * used for a property accepting a string representing a selection query. The
 * widget allows users to build selection queries easily.
 *
 * To use this widget on a property with vtkSMSelectionQueryDomain, set the
 * `panel_widget` to `selection_query` (@sa pqStandardPropertyWidgetInterface).
 *
 * @section pqSelectionQueryPropertyWidget_caveats Caveats
 *
 * Currently, this widget can faithfully present GUI for query strings that
 * follow a specific form that is generated by this property itself. Any other
 * forms, though valid queries, will not be represented properly in the GUI.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSelectionQueryPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged);

public:
  pqSelectionQueryPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqSelectionQueryPropertyWidget() override;

  ///@{
  /**
   * Get/Set the query string.
   */
  void setQuery(const QString&);
  const QString& query() const;
  ///@}

Q_SIGNALS:
  void queryChanged();

private:
  Q_DISABLE_COPY(pqSelectionQueryPropertyWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  class pqValueWidget;
  class pqQueryWidget;

  friend class pqInternals;
  friend class pqValueWidget;
  friend class pqQueryWidget;
};

#endif
