/*=========================================================================

   Program: ParaView
   Module:    pqSpinBoxDomain.cxx

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
#include "pqSpinBoxDomain.h"

// Qt includes
#include <QSpinBox>
#include <QTimer>

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

  
class pqSpinBoxDomain::pqInternal
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
  

pqSpinBoxDomain::pqSpinBoxDomain(QSpinBox* p, vtkSMProperty* prop, int index)
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
    vtkSMIntRangeDomain* drange;
    drange = vtkSMIntRangeDomain::SafeDownCast(iter->GetDomain());
    if(drange)
      {
      this->Internal->Domain = drange;
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

pqSpinBoxDomain::~pqSpinBoxDomain()
{
  delete this->Internal;
}
  
void pqSpinBoxDomain::domainChanged()
{
  if(this->Internal->MarkedForUpdate)
    {
    return;
    }

  this->Internal->MarkedForUpdate = true;
  QTimer::singleShot(0, this, SLOT(internalDomainChanged()));
}

void pqSpinBoxDomain::internalDomainChanged()
{
  QSpinBox* spinbox = qobject_cast<QSpinBox*>(this->parent());
  Q_ASSERT(spinbox != NULL);
  if(!spinbox)
    {
    return;
    }

  pqSMAdaptor::PropertyType type;
  type = pqSMAdaptor::getPropertyType(this->Internal->Property);
  QList<QVariant> range;
  if(type == pqSMAdaptor::SINGLE_ELEMENT)
    {
    range = pqSMAdaptor::getElementPropertyDomain(this->Internal->Property);
    if(range.size() == 2)
      {
      int min = range[0].toInt();
      int max = range[1].toInt();
      if(range[0].type() == QVariant::Int)
        {
        //spinbox->setSingleStep( (max - min) / 50 );  // arbitrary
        spinbox->setSingleStep(1);
        }
      else
        {
        spinbox->setSingleStep(1);
        }
      spinbox->setRange(min, max);
      }
    }
  else if(type == pqSMAdaptor::MULTIPLE_ELEMENTS)
    {
    range = pqSMAdaptor::getMultipleElementPropertyDomain(this->Internal->Property,
                                                          this->Internal->Index);
    if(range.size() == 2)
      {
      int min = range[0].toInt();
      int max = range[1].toInt();
      if(range[0].type() == QVariant::Int)
        {
        //spinbox->setSingleStep( (max - min) / 50 );  // arbitrary
        spinbox->setSingleStep(1);
        }
      else
        {
        spinbox->setSingleStep(1);
        }
      spinbox->setRange(min, max);
      }
    }
  this->Internal->MarkedForUpdate = false;
}


