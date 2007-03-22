/*=========================================================================

   Program: ParaView
   Module:    pqComboBoxDomain.cxx

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
#include "pqComboBoxDomain.h"

// Qt includes
#include <QComboBox>
#include <QTimer>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkEventQtSlotConnect.h>

// ParaView Server Manager includes
#include <vtkSMProperty.h>
#include <vtkSMDomain.h>
#include <vtkSMDomainIterator.h>
#include <vtkSMEnumerationDomain.h>
#include <vtkSMStringListDomain.h>
#include <vtkSMArrayListDomain.h>


// ParaView includes
#include <pqSMAdaptor.h>

  
class pqComboBoxDomain::pqInternal
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
  vtkSmartPointer<vtkSMDomain> Domain;
  vtkEventQtSlotConnect* Connection;
  QString DomainName;
  bool MarkedForUpdate;
};
  

pqComboBoxDomain::pqComboBoxDomain(QComboBox* p, vtkSMProperty* prop,
                                   const QString& domainName)
  : QObject(p)
{
  this->Internal = new pqInternal();
  this->Internal->Property = prop;
  this->Internal->DomainName = domainName;

  if(!domainName.isEmpty())
    {
    this->Internal->Domain = prop->GetDomain(domainName.toAscii().data());
    }
  else
    {
    // get domain
    vtkSMDomainIterator* iter = prop->NewDomainIterator();
    iter->Begin();
    while(!iter->IsAtEnd() && !this->Internal->Domain)
      {
      if(vtkSMEnumerationDomain::SafeDownCast(iter->GetDomain()) ||
         vtkSMStringListDomain::SafeDownCast(iter->GetDomain()) ||
         vtkSMArrayListDomain::SafeDownCast(iter->GetDomain()))
        {
        this->Internal->Domain = iter->GetDomain();
        }
      iter->Next();
      }
    iter->Delete();
    }

  if(this->Internal->Domain)
    {
    this->Internal->Connection->Connect(this->Internal->Domain, 
                                        vtkCommand::DomainModifiedEvent,
                                        this,
                                        SLOT(domainChanged()));
    this->internalDomainChanged();
    }
  
}

pqComboBoxDomain::~pqComboBoxDomain()
{
  delete this->Internal;
}

void pqComboBoxDomain::domainChanged()
{
  if(this->Internal->MarkedForUpdate)
    {
    return;
    }

  this->Internal->MarkedForUpdate = true;
  QTimer::singleShot(0, this, SLOT(internalDomainChanged()));
}

void pqComboBoxDomain::internalDomainChanged()
{
  QComboBox* combo = qobject_cast<QComboBox*>(this->parent());
  Q_ASSERT(combo != NULL);
  if(!combo)
    {
    return;
    }

  QList<QString> domain;
  pqSMAdaptor::PropertyType type;

  type = pqSMAdaptor::getPropertyType(this->Internal->Property);
  if(type == pqSMAdaptor::ENUMERATION)
    {
    QList<QVariant> enums;
    enums = pqSMAdaptor::getEnumerationPropertyDomain(this->Internal->Property);
    foreach(QVariant var, enums)
      {
      domain.append(var.toString());
      }
    }
  else if(type == pqSMAdaptor::FIELD_SELECTION)
    {
    if(this->Internal->DomainName == "field_list")
      {
      domain = pqSMAdaptor::getFieldSelectionModeDomain(this->Internal->Property);
      }
    else if(this->Internal->DomainName == "array_list")
      {
      domain = pqSMAdaptor::getFieldSelectionScalarDomain(this->Internal->Property);
      }
    }
  else if(type == pqSMAdaptor::PROXYSELECTION ||
          type == pqSMAdaptor::PROXYLIST)
    {
    QList<pqSMProxy> proxies = pqSMAdaptor::getProxyPropertyDomain(
      this->Internal->Property);
    foreach(vtkSMProxy* pxy, proxies)
      {
      domain.append(pxy->GetXMLLabel());
      }
    }

  // check if the domain didn't change
  QList<QString> oldDomain;

  for(int i=0; i<combo->count(); i++)
    {
    oldDomain.append(combo->itemText(i));
    }

  if(oldDomain != domain)
    {
    // save previous value to put back
    QString old = combo->currentText();
    combo->blockSignals(true);
    combo->clear();
    combo->addItems(domain);
    combo->setCurrentIndex(-1);
    combo->blockSignals(false);
    int foundOld = combo->findText(old);
    if(foundOld >= 0)
      {
      combo->setCurrentIndex(foundOld);
      }
    else
      {
      combo->setCurrentIndex(0);
      }
    }
  this->Internal->MarkedForUpdate = false;
}


