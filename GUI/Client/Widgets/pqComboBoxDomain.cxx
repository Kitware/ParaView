/*=========================================================================

   Program:   ParaQ
   Module:    pqComboBoxDomain.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
#include "pqComboBoxDomain.h"

// Qt includes
#include <QComboBox>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkEventQtSlotConnect.h>

// SM includes
#include <vtkSMProperty.h>
#include <vtkSMDomain.h>
#include <vtkSMDomainIterator.h>
#include <vtkSMEnumerationDomain.h>


// ParaQ includes
#include <pqSMAdaptor.h>

  
class pqComboBoxDomain::pqInternal
{
public:
  pqInternal()
    {
    this->Connection = vtkEventQtSlotConnect::New();
    }
  ~pqInternal()
    {
    this->Connection->Delete();
    }
  vtkSmartPointer<vtkSMProperty> Property;
  vtkSmartPointer<vtkSMDomain> Domain;
  vtkEventQtSlotConnect* Connection;
};
  

pqComboBoxDomain::pqComboBoxDomain(QComboBox* p, vtkSMProperty* prop)
  : QObject(p)
{
  this->Internal = new pqInternal();
  this->Internal->Property = prop;

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

pqComboBoxDomain::~pqComboBoxDomain()
{
  delete this->Internal;
}

void pqComboBoxDomain::domainChanged()
{
  QComboBox* combo = qobject_cast<QComboBox*>(this->parent());
  Q_ASSERT(combo != NULL);
  if(!combo)
    {
    return;
    }

  pqSMAdaptor::PropertyType type;
  type = pqSMAdaptor::getPropertyType(this->Internal->Property);
  if(type == pqSMAdaptor::ENUMERATION)
    {
    QList<QVariant> enums;
    enums = pqSMAdaptor::getEnumerationPropertyDomain(this->Internal->Property);
    // save previous value to put back
    QString old = combo->currentText();
    combo->clear();
    foreach(QVariant var, enums)
      {
      combo->addItem(var.toString());
      }
    int foundOld = combo->findText(old);
    combo->setCurrentIndex(foundOld);
    }
}


