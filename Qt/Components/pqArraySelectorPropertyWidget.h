/*=========================================================================

   Program: ParaView
   Module:  pqArraySelectorPropertyWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqArraySelectorPropertyWidget_h
#define pqArraySelectorPropertyWidget_h

#include "pqPropertyWidget.h"
#include <QScopedPointer> // needed for ivar

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

public Q_SLOTS:
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

private:
  Q_DISABLE_COPY(pqArraySelectorPropertyWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  class PropertyLinksConnection;
};

#endif
