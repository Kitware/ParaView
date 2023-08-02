// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqArraySelectorPropertyWidget_h
#define pqArraySelectorPropertyWidget_h

#include "pqPropertyWidget.h"
#include <QPair>          // need for ctor arg
#include <QScopedPointer> // needed for ivar

#include <initializer_list> // need for ctor arg

/**
 * @class pqArraySelectorPropertyWidget
 * @brief pqPropertyWidget subclass for properties with vtkSMArrayListDomain.
 *
 * pqArraySelectorPropertyWidget is intended to be used for
 * vtkSMStringVectorProperty instances that have a vtkSMArrayListDomain domain
 * and want to show a single combo-box to allow the user to choose the array to use.
 *
 * We support non-repeatable string-vector property with a 1, 2, or 5 elements.
 * When 1 element is present, we interpret the property value as the name of
 * chosen array, thus the user won't be able to pick array association. While
 * for 2 and 5 element properties the array association and name can be picked.
 *
 * The list of available arrays is built using the vtkSMArrayListDomain and
 * updated anytime the domain is updated. If the currently chosen value is no
 * longer in the domain, we will preserve it and flag it by adding a `(?)`
 * suffix to the displayed label.
 *
 * `pqStringVectorPropertyWidget::createWidget` instantiates this for
 * any string vector property with a vtkSMArrayListDomain that is not
 * repeatable.
 *
 * Pre-defined entries can be provided with the KnownArrays argument when constructing
 * the widget.
 */
class PQCOMPONENTS_EXPORT pqArraySelectorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

  Q_PROPERTY(QList<QVariant> array READ array WRITE setArray);
  Q_PROPERTY(QString arrayName READ arrayName WRITE setArrayName);

public:
  pqArraySelectorPropertyWidget(
    vtkSMProperty* smproperty, vtkSMProxy* smproxy, QWidget* parent = nullptr);
  pqArraySelectorPropertyWidget(vtkSMProperty* smproperty, vtkSMProxy* smproxy,
    std::initializer_list<QPair<int, QString>> knownArrays, QWidget* parent = nullptr);
  ~pqArraySelectorPropertyWidget() override;

  /**
   * Returns the chosen array name.
   */
  QString arrayName() const;

  /**
   * Returns the chosen array association.
   */
  int arrayAssociation() const;

  /**
   * Returns the `{association, name}` for the chosen array.
   */
  QList<QVariant> array() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the chosen array name and association.
   */
  void setArray(int assoc, const QString& val);

  /**
   * A setArray overload useful to expose the setArray using Qt's property
   * system.  `val` must be a two-tuple.
   */
  void setArray(const QList<QVariant>& val);

  /**
   * Set the array name without caring for the array association.
   * In general, this is only meant to be used for connecting to a SMProperty
   * which only lets the user choose the array name and not the association.
   */
  void setArrayName(const QString& name);

Q_SIGNALS:
  void arrayChanged();

private Q_SLOTS:
  void domainModified();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqArraySelectorPropertyWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  class PropertyLinksConnection;
};

#endif
