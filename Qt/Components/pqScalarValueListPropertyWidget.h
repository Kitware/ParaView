// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqScalarValueListPropertyWidget_h
#define pqScalarValueListPropertyWidget_h

#include "pqPropertyWidget.h"

#include <QVariant>
#include <string>
#include <vector>

class QListWidgetItem;
class vtkPVXMLElement;
class vtkSMDoubleRangeDomain;
class vtkSMIntRangeDomain;
class vtkSMTimeStepsDomain;

/**
 * pqScalarValueListPropertyWidget provides a table widget to which users are
 * add values e.g. for IsoValues for the Contour filter.
 *
 * This widget supports the `AllowRestoreDefaults` hints, which adds a button that
 * allow users to restore the default value of the property.
 */
class PQCOMPONENTS_EXPORT pqScalarValueListPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QVariantList scalars READ scalars WRITE setScalars)

  typedef pqPropertyWidget Superclass;

public:
  pqScalarValueListPropertyWidget(
    vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = nullptr);
  ~pqScalarValueListPropertyWidget() override;

  void setScalars(const QVariantList& scalars);
  QVariantList scalars() const;

  ///@{
  /**
   * Sets range domain that will be used to initialize the scalar range.
   * vtkSMTimeStepsDomain does have a concept of min and max that can be used as a range.
   */
  void setRangeDomain(vtkSMDoubleRangeDomain* smRangeDomain);
  void setRangeDomain(vtkSMIntRangeDomain* smRangeDomain);
  void setRangeDomain(vtkSMTimeStepsDomain* timestepsDomain);
  ///@}

  void setShowLabels(bool);
  void setLabels(const std::vector<std::string>& labels);

Q_SIGNALS:
  void scalarsChanged();

private Q_SLOTS:
  void smRangeModified();

  /**
   * slots called when corresponding buttons are clicked.
   */
  void add();
  void addRange();
  void remove();
  void removeAll();
  void editPastLastRow();
  void restoreDefaults();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqScalarValueListPropertyWidget)

  bool getRange(double& range_min, double& range_max);
  bool getRange(int& range_min, int& range_max);

  class pqInternals;
  pqInternals* Internals;
};

#endif // pqScalarValueListPropertyWidget_h
