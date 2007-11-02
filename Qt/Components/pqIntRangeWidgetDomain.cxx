/*=========================================================================

   Program: ParaView
   Module:    pqIntRangeWidgetDomain.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqIntRangeWidgetDomain.h"

// Qt includes
#include <QTimer>

// Widgets includes
#include "pqIntRangeWidget.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkEventQtSlotConnect.h>

// ParaView Server Manager includes
#include <vtkSMProperty.h>
#include <vtkSMDomain.h>
#include <vtkSMDomainIterator.h>
#include <vtkSMIntRangeDomain.h>


// ParaView includes
#include <pqSMAdaptor.h>

  
class pqIntRangeWidgetDomain::pqInternal
{
public:
  pqInternal()
    {
    this->Connection = vtkEventQtSlotConnect::New();
    this->MarkedForUpdate = false;
    }
  ~pqInternal()
    {
    this->Connection->Delete();
    }
  vtkSmartPointer<vtkSMProperty> Property;
  int Index;
  vtkSmartPointer<vtkSMDomain> Domain;
  vtkEventQtSlotConnect* Connection;
  bool MarkedForUpdate;
};
  

pqIntRangeWidgetDomain::pqIntRangeWidgetDomain(pqIntRangeWidget* p, vtkSMProperty* prop, int index)
  : QObject(p)
{
  this->Internal = new pqInternal();
  this->Internal->Property = prop;
  this->Internal->Index = index;

  // get domain
  vtkSMDomainIterator* iter = prop->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd() && !this->Internal->Domain)
    {
    vtkSMIntRangeDomain* irange;
    irange = vtkSMIntRangeDomain::SafeDownCast(iter->GetDomain());
    if(irange)
      {
      this->Internal->Domain = irange;
      }
    iter->Next();
    }
  iter->Delete();

  if(this->Internal->Domain)
    {
    this->Internal->Connection->Connect(this->Internal->Domain, 
                                        vtkCommand::DomainModifiedEvent,
                                        this,
                                        SLOT(domainChanged()));
    this->internalDomainChanged();
    }
  
}


pqIntRangeWidgetDomain::~pqIntRangeWidgetDomain()
{
  delete this->Internal;
}

void pqIntRangeWidgetDomain::domainChanged()
{
  if(this->Internal->MarkedForUpdate)
    {
    return;
    }

  this->Internal->MarkedForUpdate = true;
  QTimer::singleShot(0, this, SLOT(internalDomainChanged()));
}

//-----------------------------------------------------------------------------
void pqIntRangeWidgetDomain::setRange(int min, int max)
{
  pqIntRangeWidget* range = this->getRangeWidget();
  if(range)
    {
    range->setMinimum(min);
    range->setMaximum(max);
    if(this->Internal->Domain->GetClassName() ==
      QString("vtkSMIntRangeDomain"))
      {
      range->setStrictRange(min, max);
      }
    }
}

//-----------------------------------------------------------------------------
pqIntRangeWidget* pqIntRangeWidgetDomain::getRangeWidget() const
{
  pqIntRangeWidget* range = qobject_cast<pqIntRangeWidget*>(this->parent());
  Q_ASSERT(range != NULL);
  return range;
}

//-----------------------------------------------------------------------------
void pqIntRangeWidgetDomain::internalDomainChanged()
{
  pqSMAdaptor::PropertyType type;
  type = pqSMAdaptor::getPropertyType(this->Internal->Property);
  QList<QVariant> range;
  if(type == pqSMAdaptor::SINGLE_ELEMENT)
    {
    range = pqSMAdaptor::getElementPropertyDomain(this->Internal->Property);
    if(range.size() == 2 && range[0].isValid() && range[1].isValid())
      {
      int min = range[0].toInt();
      int max = range[1].toInt();
      this->setRange(min, max);
      }
    }
  else if(type == pqSMAdaptor::MULTIPLE_ELEMENTS)
    {
    range = pqSMAdaptor::getMultipleElementPropertyDomain(this->Internal->Property,
                                                          this->Internal->Index);
    if(range.size() == 2 && range[0].isValid() && range[1].isValid())
      {
      int min = range[0].toInt();
      int max = range[1].toInt();
      this->setRange(min, max);
      }
    }
  this->Internal->MarkedForUpdate = false;
}


