// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqWidgetRangeDomain_h
#define pqWidgetRangeDomain_h

#include "pqComponentsModule.h"
#include <QObject>

class vtkSMProperty;
class QWidget;

/**
 * observes the domain for a property and updates the minimum and/or maximum
 * properties of the object
 */
class PQCOMPONENTS_EXPORT pqWidgetRangeDomain : public QObject
{
  Q_OBJECT
public:
  pqWidgetRangeDomain(QWidget* p, const QString& minProp, const QString& maxProp,
    vtkSMProperty* prop, int index = -1);
  ~pqWidgetRangeDomain() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void domainChanged();
protected Q_SLOTS:
  void internalDomainChanged();

protected: // NOLINT(readability-redundant-access-specifiers)
  virtual void setRange(QVariant min, QVariant max);

  QWidget* getWidget() const;

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
