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

// ParaView includes
#include "pqSMAdaptor.h"
#include "pqTimer.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMDomain.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMStringVectorProperty.h"

pqFieldSelectionAdaptor::pqFieldSelectionAdaptor(QComboBox* p, vtkSMProperty* prop)
  : QObject(p)
  , Property(prop)
  , MarkedForUpdate(false)
  , IsGettingAllDomains(false)
{
  this->Connection = vtkEventQtSlotConnect::New();

  // resize selection to two strings
  this->Selection.append("");
  this->Selection.append("");

  if (p && pqSMAdaptor::getPropertyType(prop) == pqSMAdaptor::FIELD_SELECTION)
  {
    this->AttributeModeDomain = prop->GetDomain("field_list");
    this->ScalarDomain = prop->GetDomain("array_list");

    this->internalDomainChanged();

    this->Connection->Connect(
      this->AttributeModeDomain, vtkCommand::DomainModifiedEvent, this, SLOT(domainChanged()));

    this->Connection->Connect(
      this->ScalarDomain, vtkCommand::DomainModifiedEvent, this, SLOT(domainChanged()));

    this->Connection->Connect(this->AttributeModeDomain, vtkCommand::DomainModifiedEvent, this,
      SLOT(blockDomainModified(vtkObject*, unsigned long, void*, void*, vtkCommand*)), NULL, 1.0);

    this->Connection->Connect(this->ScalarDomain, vtkCommand::DomainModifiedEvent, this,
      SLOT(blockDomainModified(vtkObject*, unsigned long, void*, void*, vtkCommand*)), NULL, 1.0);

    QObject::connect(p, SIGNAL(currentIndexChanged(int)), this, SLOT(indexChanged(int)));
  }
}

pqFieldSelectionAdaptor::~pqFieldSelectionAdaptor()
{
  this->Connection->Delete();
}

void pqFieldSelectionAdaptor::setSelection(const QStringList& sel)
{
  if (sel.size() != 2)
  {
    return;
  }

  if (sel != this->Selection)
  {
    this->Selection = sel;
    this->updateGUI();
    emit this->selectionChanged();
  }
}

QStringList pqFieldSelectionAdaptor::selection() const
{
  return this->Selection;
}

QString pqFieldSelectionAdaptor::attributeMode() const
{
  return this->Selection[0];
}

QString pqFieldSelectionAdaptor::scalar() const
{
  return this->Selection[1];
}

void pqFieldSelectionAdaptor::setAttributeMode(const QString& mode)
{
  this->setAttributeModeAndScalar(mode, this->scalar());
}

void pqFieldSelectionAdaptor::setScalar(const QString& sc)
{
  this->setAttributeModeAndScalar(this->attributeMode(), sc);
}

void pqFieldSelectionAdaptor::setAttributeModeAndScalar(const QString& m, const QString& s)
{
  this->setSelection(QStringList() << m << s);
}

void pqFieldSelectionAdaptor::updateGUI()
{
  QComboBox* combo = qobject_cast<QComboBox*>(this->parent());
  if (combo)
  {
    int num = combo->count();
    for (int i = 0; i < num; i++)
    {
      QStringList array = combo->itemData(i).toStringList();
      if (array == this->Selection)
      {
        if (combo->currentIndex() != i)
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
  if (combo)
  {
    this->setSelection(combo->itemData(index).toStringList());
  }
}

void pqFieldSelectionAdaptor::domainChanged()
{
  if (this->MarkedForUpdate)
  {
    return;
  }

  this->MarkedForUpdate = true;
  pqTimer::singleShot(0, this, SLOT(internalDomainChanged()));
}

void pqFieldSelectionAdaptor::blockDomainModified(
  vtkObject*, unsigned long, void*, void*, vtkCommand* cmd)
{
  if (this->IsGettingAllDomains)
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
  if (!combo)
  {
    return;
  }

  QPixmap cellPixmap(":/pqWidgets/Icons/pqCellData16.png");
  QPixmap pointPixmap(":/pqWidgets/Icons/pqPointData16.png");
  QPixmap globalPixmap(":/pqWidgets/Icons/pqGlobalData16.png");
  QPixmap rowPixmap(":/pqWidgets/Icons/pqSpreadsheet16.png");
  QPixmap edgePixmap(":/pqWidgets/Icons/pqEdgeCenterData16.png");

  vtkSMArrayListDomain* ald =
    vtkSMArrayListDomain::SafeDownCast(this->Property->GetDomain("array_list"));
  vtkSMEnumerationDomain* fld =
    vtkSMEnumerationDomain::SafeDownCast(this->Property->GetDomain("field_list"));

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
    // Refer to vtkSMArrayListDomain. FieldAssociation is the value to use on
    // the property, while DomainAssociation is the value to use for showing
    // icon/text etc.
    int field_association = ald->GetFieldAssociation(array_idx);
    int icon_association = ald->GetDomainAssociation(array_idx);
    switch (icon_association)
    {
      case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        pix = &cellPixmap;
        break;

      case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
        pix = &pointPixmap;
        break;

      case vtkDataObject::FIELD_ASSOCIATION_NONE:
        pix = &globalPixmap;
        break;

      case vtkDataObject::FIELD_ASSOCIATION_ROWS:
        pix = &rowPixmap;
        break;

      case vtkDataObject::FIELD_ASSOCIATION_EDGES:
        pix = &edgePixmap;
        break;
    }

    QString arrayName = array.first;
    QStringList data;
    data << fld->GetEntryTextForValue(field_association) << arrayName;
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
    if (data == this->selection())
    {
      newIndex = array_idx;
    }
    array_idx++;
  }
  combo->setCurrentIndex(-1);
  combo->blockSignals(false);
  if (newIndex != -1)
  {
    combo->setCurrentIndex(newIndex);
  }
  else
  {
    combo->setCurrentIndex(0);
  }

  this->MarkedForUpdate = false;
}
