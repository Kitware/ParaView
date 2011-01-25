/*=========================================================================

   Program: ParaView
   Module:    pqFieldSelectionAdaptor.cxx

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

// self includes
#include "pqFieldSelectionAdaptor.h"

// Qt includes
#include <QComboBox>
#include <QTimer>

// ParaView includes
#include "pqSMAdaptor.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMDomain.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMStringVectorProperty.h"

pqFieldSelectionAdaptor::pqFieldSelectionAdaptor(QComboBox* p,
                                 vtkSMProperty* prop)
  : QObject(p), Property(prop), 
    MarkedForUpdate(false), IsGettingAllDomains(false)
{
  this->Connection = vtkEventQtSlotConnect::New();

  if(p && pqSMAdaptor::getPropertyType(prop) == pqSMAdaptor::FIELD_SELECTION)
    {
    this->AttributeModeDomain = prop->GetDomain("field_list");
    this->ScalarDomain = prop->GetDomain("array_list");
    
    this->internalDomainChanged();

    this->Connection->Connect(this->AttributeModeDomain,
                              vtkCommand::DomainModifiedEvent,
                              this,
                              SLOT(domainChanged()));
    
    this->Connection->Connect(this->ScalarDomain,
                              vtkCommand::DomainModifiedEvent,
                              this,
                              SLOT(domainChanged()));
    
    this->Connection->Connect(this->AttributeModeDomain,
                              vtkCommand::DomainModifiedEvent,
                              this,
                              SLOT(blockDomainModified(vtkObject*, unsigned long,void*, void*, vtkCommand*)),
                              NULL, 1.0);
    
    this->Connection->Connect(this->ScalarDomain,
                              vtkCommand::DomainModifiedEvent,
                              this,
                              SLOT(blockDomainModified(vtkObject*, unsigned long,void*, void*, vtkCommand*)),
                              NULL, 1.0);
    
    QObject::connect(p, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(indexChanged(int)));

    }
}

pqFieldSelectionAdaptor::~pqFieldSelectionAdaptor()
{
  this->Connection->Delete();
}


QString pqFieldSelectionAdaptor::attributeMode() const
{
  return this->AttributeMode;
}

QString pqFieldSelectionAdaptor::scalar() const
{
  return this->Scalar;
}

void pqFieldSelectionAdaptor::setAttributeMode(const QString& mode)
{
  this->setAttributeModeAndScalar(mode, this->Scalar);
}

void pqFieldSelectionAdaptor::setScalar(const QString& sc)
{
  this->setAttributeModeAndScalar(this->AttributeMode, sc);
}

void pqFieldSelectionAdaptor::setAttributeModeAndScalar(
         const QString& mode, const QString& sc)
{
  if(mode != this->AttributeMode || sc != this->Scalar)
    {
    this->AttributeMode = mode;
    this->Scalar = sc;
    this->updateGUI();
    emit this->selectionChanged();
    }
}

void pqFieldSelectionAdaptor::updateGUI()
{
  QComboBox* combo = qobject_cast<QComboBox*>(this->parent());
  if(combo)
    {
    int num = combo->count();
    for (int i=0; i<num; i++)
      {
      QStringList array = combo->itemData(i).toStringList();
      if (array[0] == this->AttributeMode && array[1] == this->Scalar)
        {
        if(combo->currentIndex() != i)
          {
          combo->setCurrentIndex(i);
          }
        break;
        }
      }
    }
}
  
void pqFieldSelectionAdaptor::indexChanged(int index)
{
  QComboBox* combo = qobject_cast<QComboBox*>(this->parent());
  if(combo)
    {
    QStringList array = combo->itemData(index).toStringList();

    // get attribute mode type
    QString mode = array[0];
    
    // get scalar
    QString sc = array[1];

    this->setAttributeModeAndScalar(mode, sc);
    }
}

void pqFieldSelectionAdaptor::domainChanged()
{
  if(this->MarkedForUpdate)
    {
    return;
    }

  this->MarkedForUpdate = true;
  QTimer::singleShot(0, this, SLOT(internalDomainChanged()));
}
  
void pqFieldSelectionAdaptor::blockDomainModified(vtkObject*, unsigned long, 
                           void*, void*, vtkCommand* cmd)
{
  if(this->IsGettingAllDomains)
    {
    // don't let anyone else know this domain is changing (because it really isn't)
    // and we're going to put it back when we're done
    cmd->SetAbortFlag(1);
    }
}

void pqFieldSelectionAdaptor::internalDomainChanged()
{
  QComboBox* combo = qobject_cast<QComboBox*>(this->parent());
  Q_ASSERT(combo != NULL);
  if(!combo)
    {
    return;
    }
  
  QPixmap cellPixmap(":/pqWidgets/Icons/pqCellData16.png");
  QPixmap pointPixmap(":/pqWidgets/Icons/pqPointData16.png");


  QVariant originalData;
  if(combo->currentIndex() >= 0)
    {
    originalData = combo->itemData(combo->currentIndex());
    }

  vtkSMArrayListDomain* ald = vtkSMArrayListDomain::SafeDownCast(
    this->Property->GetDomain("array_list"));
  vtkSMEnumerationDomain* fld = vtkSMEnumerationDomain::SafeDownCast(
    this->Property->GetDomain("field_list"));

  this->IsGettingAllDomains = true;
  QList<QPair<QString, bool> > arrays = 
    pqSMAdaptor::getFieldSelectionScalarDomainWithPartialArrays(this->Property);
  this->IsGettingAllDomains = false;

  combo->blockSignals(true);
  combo->clear();
  int newIndex = -1;
  int array_idx = 0;
  QPair<QString, bool> array;
  foreach (array, arrays)
    {
    QPixmap* pix = 0;
    int field_association = ald->GetFieldAssociation(array_idx);
    //Because of auto conversion we want the domain type association instead of the properties
    //This makes sure we don't use a point property as an cell property, but instead
    //convert it with vtkPVPostFilter.
    //If this is removed the use case where you have point & cell properties with the same
    //name fails. It will use the cell arrays on the point data, instead of grabbing the point array
    int domain_association = ald->GetDomainAssociation(array_idx);
    switch (field_association)
      {
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      pix = &cellPixmap;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      pix = &pointPixmap;
      break;
      }

    QString arrayName = array.first;
    QStringList data;
    data << fld->GetEntryTextForValue(domain_association)
         << arrayName
         << (array.second? "1" : "0");
    if (array.second)
      {
      arrayName += " (partial)";
      }

    if (pix)
      {
      combo->addItem(QIcon(*pix), arrayName, QVariant(data));
      }
    else
      {
      combo->addItem(arrayName, QVariant(data));
      }
    if (QVariant(data) == originalData)
      {
      newIndex = array_idx;
      }
    array_idx++;
    }
  combo->setCurrentIndex(-1);
  combo->blockSignals(false);
  if(newIndex != -1)
    {
    combo->setCurrentIndex(newIndex);
    }
  else
    {
    combo->setCurrentIndex(0);
    }

  this->MarkedForUpdate = false;
}

