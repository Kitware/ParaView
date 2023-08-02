// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqArrayListDomain_h
#define pqArrayListDomain_h

#include "pqComponentsModule.h"
#include <QObject>

class QWidget;
class vtkSMProperty;
class vtkSMProxy;
class vtkSMDomain;

/**
 * pqArrayListDomain is used to connect a widget showing a selection of arrays
 * with its vtkSMArrayListDomain. Whenever the vtkSMArrayListDomain changes,
 * the widget is "reset" to update using the property's new domain. This is
 * useful for DescriptiveStatistics panel, for example. Whenever the attribute
 * selection changes (i.e. user switches from cell-data to point-data), we need
 * to update the widget's contents to show the list of array in the
 * corresponding attribute. This class takes care of that.
 */
class PQCOMPONENTS_EXPORT pqArrayListDomain : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqArrayListDomain(QWidget* selectorWidget, const QString& qproperty, vtkSMProxy* proxy,
    vtkSMProperty* smproperty, vtkSMDomain* domain);
  ~pqArrayListDomain() override;

private Q_SLOTS:
  void domainChanged();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqArrayListDomain)
  class pqInternals;
  pqInternals* Internals;
};

#endif
