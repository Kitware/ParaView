// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// self include
#include "pqWidgetRangeDomain.h"

// Qt includes
#include <QWidget>

// VTK includes
#include <vtkEventQtSlotConnect.h>
#include <vtkSmartPointer.h>

// ParaView Server Manager includes
#include <vtkSMDomain.h>
#include <vtkSMDomainIterator.h>
#include <vtkSMDoubleRangeDomain.h>
#include <vtkSMEnumerationDomain.h>
#include <vtkSMIntRangeDomain.h>
#include <vtkSMProperty.h>

// ParaView includes
#include <pqSMAdaptor.h>
#include <pqTimer.h>

#include <cassert>

class pqWidgetRangeDomain::pqInternal
{
public:
  pqInternal()
  {
    this->Connection = vtkEventQtSlotConnect::New();
    this->MarkedForUpdate = false;
  }
  ~pqInternal() { this->Connection->Delete(); }
  QString MinProp;
  QString MaxProp;
  vtkSmartPointer<vtkSMProperty> Property;
  int Index;
  vtkSmartPointer<vtkSMDomain> Domain;
  vtkEventQtSlotConnect* Connection;
  bool MarkedForUpdate;
};

pqWidgetRangeDomain::pqWidgetRangeDomain(
  QWidget* p, const QString& minProp, const QString& maxProp, vtkSMProperty* prop, int index)
  : QObject(p)
{
  this->Internal = new pqInternal();
  this->Internal->MinProp = minProp;
  this->Internal->MaxProp = maxProp;
  this->Internal->Property = prop;
  this->Internal->Index = index;

  // get domain
  vtkSMDomainIterator* iter = prop->NewDomainIterator();
  iter->Begin();
  while (!iter->IsAtEnd() && !this->Internal->Domain)
  {
    vtkSMEnumerationDomain* enumeration;
    enumeration = vtkSMEnumerationDomain::SafeDownCast(iter->GetDomain());
    if (enumeration)
    {
      this->Internal->Domain = enumeration;
    }

    vtkSMDoubleRangeDomain* drange;
    drange = vtkSMDoubleRangeDomain::SafeDownCast(iter->GetDomain());
    if (drange)
    {
      this->Internal->Domain = drange;
    }

    vtkSMIntRangeDomain* irange;
    irange = vtkSMIntRangeDomain::SafeDownCast(iter->GetDomain());
    if (irange)
    {
      this->Internal->Domain = irange;
    }
    iter->Next();
  }
  iter->Delete();

  if (this->Internal->Domain)
  {
    this->Internal->Connection->Connect(
      this->Internal->Domain, vtkCommand::DomainModifiedEvent, this, SLOT(domainChanged()));
    this->internalDomainChanged();
  }
}

pqWidgetRangeDomain::~pqWidgetRangeDomain()
{
  delete this->Internal;
}

void pqWidgetRangeDomain::domainChanged()
{
  if (this->Internal->MarkedForUpdate)
  {
    return;
  }

  this->Internal->MarkedForUpdate = true;
  pqTimer::singleShot(0, this, SLOT(internalDomainChanged()));
}

//-----------------------------------------------------------------------------
void pqWidgetRangeDomain::setRange(QVariant min, QVariant max)
{
  QWidget* range = this->getWidget();
  if (range)
  {
    if (!this->Internal->MinProp.isEmpty())
    {
      range->setProperty(this->Internal->MinProp.toUtf8().data(), min);
    }
    if (!this->Internal->MaxProp.isEmpty())
    {
      range->setProperty(this->Internal->MaxProp.toUtf8().data(), max);
    }
  }
}

//-----------------------------------------------------------------------------
QWidget* pqWidgetRangeDomain::getWidget() const
{
  QWidget* range = qobject_cast<QWidget*>(this->parent());
  assert(range != nullptr);
  return range;
}

//-----------------------------------------------------------------------------
void pqWidgetRangeDomain::internalDomainChanged()
{
  pqSMAdaptor::PropertyType type;
  type = pqSMAdaptor::getPropertyType(this->Internal->Property);
  int index = type == pqSMAdaptor::SINGLE_ELEMENT ? 0 : this->Internal->Index;

  QList<QVariant> range =
    pqSMAdaptor::getMultipleElementPropertyDomain(this->Internal->Property, index);

  if (range.size() == 2)
  {
    this->setRange(range[0], range[1]);
  }

  this->Internal->MarkedForUpdate = false;
}
