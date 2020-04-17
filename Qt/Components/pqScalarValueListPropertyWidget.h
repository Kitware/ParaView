/*=========================================================================

   Program: ParaView
   Module: pqScalarValueListPropertyWidget.h

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef _pqScalarValueListPropertyWidget_h
#define _pqScalarValueListPropertyWidget_h

#include "pqPropertyWidget.h"

#include <QVariant>

class QListWidgetItem;
class vtkSMDoubleRangeDomain;
class vtkSMIntRangeDomain;

/**
* pqScalarValueListPropertyWidget provides a table widget to which users are
* add values e.g. for IsoValues for the Contour filter.
*/
class PQCOMPONENTS_EXPORT pqScalarValueListPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QVariantList scalars READ scalars WRITE setScalars)

  typedef pqPropertyWidget Superclass;

public:
  pqScalarValueListPropertyWidget(vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = 0);
  ~pqScalarValueListPropertyWidget() override;

  void setScalars(const QVariantList& scalars);
  QVariantList scalars() const;

  /**
  * Sets range domain that will be used to initialize the scalar range.
  */
  void setRangeDomain(vtkSMDoubleRangeDomain* smRangeDomain);
  void setRangeDomain(vtkSMIntRangeDomain* smRangeDomain);

  void setShowLabels(bool);
  void setLabels(std::vector<const char*>&);

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

private:
  Q_DISABLE_COPY(pqScalarValueListPropertyWidget)

  bool getRange(double& range_min, double& range_max);
  bool getRange(int& range_min, int& range_max);

  class pqInternals;
  pqInternals* Internals;
};

#endif // _pqScalarValueListPropertyWidget_h
