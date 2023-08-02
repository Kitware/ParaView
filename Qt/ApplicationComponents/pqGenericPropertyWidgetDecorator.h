// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqGenericPropertyWidgetDecorator_h
#define pqGenericPropertyWidgetDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"

#include <QScopedPointer>

/**
 * pqGenericPropertyWidgetDecorator is a pqPropertyWidgetDecorator that
 * supports multiple common use cases from a pqPropertyWidgetDecorator.
 * The use cases supported are as follows:
 * \li 1. enabling the pqPropertyWidget when the value of another
 *   property element matches a specific value (disabling otherwise).
 * \li 2. similar to 1, except instead of enabling/disabling the widget is made
 *   "default" when the values match and "advanced" otherwise.
 * \li 3. enabling the pqPropertyWidget when the array named in the property
 *   has a specified number of components.
 * \li 4. as well as "inverse" of all the above i.e. when the value doesn't
 *   match the specified value.
 * Example usages:
 * \li VectorScaleMode, Stride, Seed, MaximumNumberOfSamplePoints properties on the Glyph proxy.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqGenericPropertyWidgetDecorator
  : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqGenericPropertyWidgetDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqGenericPropertyWidgetDecorator() override;

  /**
   * Methods overridden from pqPropertyWidget.
   */
  bool canShowWidget(bool show_advanced) const override;
  bool enableWidget() const override;

private Q_SLOTS:
  void updateState();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqGenericPropertyWidgetDecorator)

  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};

#endif
