/*=========================================================================

   Program: ParaView
   Module:    pqWidgetRangeDomain.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
      range->setProperty(this->Internal->MinProp.toLocal8Bit().data(), min);
    }
    if (!this->Internal->MaxProp.isEmpty())
    {
      range->setProperty(this->Internal->MaxProp.toLocal8Bit().data(), max);
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
