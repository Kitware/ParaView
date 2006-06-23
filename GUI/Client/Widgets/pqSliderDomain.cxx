/*=========================================================================

   Program: ParaView
   Module:    pqSliderDomain.cxx

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
#include "pqSliderDomain.h"

// Qt includes
#include <QSlider>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkEventQtSlotConnect.h>

// ParaView Server Manager includes
#include <vtkSMProperty.h>
#include <vtkSMDomain.h>
#include <vtkSMDomainIterator.h>
#include <vtkSMEnumerationDomain.h>
#include <vtkSMDoubleRangeDomain.h>
#include <vtkSMIntRangeDomain.h>


// ParaView Server Manager includes
#include <pqSMAdaptor.h>

  
class pqSliderDomain::pqInternal
{
public:
  pqInternal()
    {
    this->ScaleFactor = 1.0;
    this->Connection = vtkEventQtSlotConnect::New();
    }
  ~pqInternal()
    {
    this->Connection->Delete();
    }
  vtkSmartPointer<vtkSMProperty> Property;
  int Index;
  vtkSmartPointer<vtkSMDomain> Domain;
  vtkEventQtSlotConnect* Connection;
  double ScaleFactor;
};
  

pqSliderDomain::pqSliderDomain(QSlider* p, vtkSMProperty* prop, int index)
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
    vtkSMEnumerationDomain* enumeration;
    enumeration = vtkSMEnumerationDomain::SafeDownCast(iter->GetDomain());
    if(enumeration)
      {
      this->Internal->Domain = enumeration;
      }

    vtkSMDoubleRangeDomain* drange;
    drange = vtkSMDoubleRangeDomain::SafeDownCast(iter->GetDomain());
    if(drange)
      {
      this->Internal->Domain = drange;
      }
    
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
                                        vtkCommand::ModifiedEvent,
                                        this,
                                        SLOT(domainChanged()));
    }
}

pqSliderDomain::~pqSliderDomain()
{
  delete this->Internal;
}
  
void pqSliderDomain::setScaleFactor(double scale)
{
  this->Internal->ScaleFactor = scale;
}

double pqSliderDomain::scaleFactor() const
{
  return this->Internal->ScaleFactor;
}


void pqSliderDomain::domainChanged()
{
  QSlider* slider = qobject_cast<QSlider*>(this->parent());
  Q_ASSERT(slider != NULL);
  if(!slider)
    {
    return;
    }

  pqSMAdaptor::PropertyType type;
  type = pqSMAdaptor::getPropertyType(this->Internal->Property);
  if(type == pqSMAdaptor::SINGLE_ELEMENT)
    {
    QList<QVariant> range;
    range = pqSMAdaptor::getElementPropertyDomain(this->Internal->Property);
    if(range.size() == 2)
      {
      double min = range[0].toDouble() * this->Internal->ScaleFactor;
      double max = range[1].toDouble() * this->Internal->ScaleFactor;
      slider->setMinimum(qRound(min));
      slider->setMaximum(qRound(max));
      }
    }
  else if(type == pqSMAdaptor::MULTIPLE_ELEMENTS)
    {
    QList<QVariant> range;
    range = pqSMAdaptor::getMultipleElementPropertyDomain(this->Internal->Property,
                                                          this->Internal->Index);
    if(range.size() == 2)
      {
      double min = range[0].toDouble() * this->Internal->ScaleFactor;
      double max = range[1].toDouble() * this->Internal->ScaleFactor;
      slider->setMinimum(qRound(min));
      slider->setMaximum(qRound(max));
      }
    }
}


