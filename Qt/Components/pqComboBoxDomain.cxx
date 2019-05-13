/*=========================================================================

   Program: ParaView
   Module:    pqComboBoxDomain.cxx

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
#include "pqComboBoxDomain.h"

// Qt includes
#include <QComboBox>

// VTK includes
#include <vtkDataObject.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkSMFieldDataDomain.h>
#include <vtkSmartPointer.h>

// ParaView Server Manager includes
#include <vtkSMArrayListDomain.h>
#include <vtkSMDomain.h>
#include <vtkSMDomainIterator.h>
#include <vtkSMEnumerationDomain.h>
#include <vtkSMProperty.h>
#include <vtkSMStringListDomain.h>

// ParaView includes
#include <pqSMAdaptor.h>

#include <cassert>

namespace
{
QIcon get_icon(int assoc)
{
  switch (assoc)
  {
    case vtkDataObject::POINT:
      return QIcon(":/pqWidgets/Icons/pqPointData16.png");
    case vtkDataObject::CELL:
      return QIcon(":/pqWidgets/Icons/pqCellData16.png");
    case vtkDataObject::FIELD:
      return QIcon(":/pqWidgets/Icons/pqGlobalData16.png");
    case vtkDataObject::VERTEX:
      return QIcon(":/pqWidgets/Icons/pqPointData16.png");
    case vtkDataObject::EDGE:
      return QIcon(":/pqWidgets/Icons/pqEdgeCenterData16.png");
    case vtkDataObject::ROW:
      return QIcon(":/pqWidgets/Icons/pqSpreadsheet16.png");
    default:
      return QIcon();
  }
}
}

class pqComboBoxDomain::pqInternal
{
public:
  pqInternal()
  {
    this->Connection = vtkEventQtSlotConnect::New();
    this->MarkedForUpdate = false;
  }
  ~pqInternal() { this->Connection->Delete(); }
  vtkSmartPointer<vtkSMProperty> Property;
  vtkSmartPointer<vtkSMDomain> Domain;
  vtkEventQtSlotConnect* Connection;
  QStringList UserStrings;
  bool MarkedForUpdate;
};

pqComboBoxDomain::pqComboBoxDomain(QComboBox* p, vtkSMProperty* prop, vtkSMDomain* domain)
  : QObject(p)
{
  this->Internal = new pqInternal();
  this->Internal->Property = prop;
  this->Internal->Domain = domain;

  if (!this->Internal->Domain)
  {
    // get domain
    vtkSMDomainIterator* iter = prop->NewDomainIterator();
    iter->Begin();
    while (!iter->IsAtEnd() && !this->Internal->Domain)
    {
      if (vtkSMEnumerationDomain::SafeDownCast(iter->GetDomain()) ||
        vtkSMStringListDomain::SafeDownCast(iter->GetDomain()) ||
        vtkSMArrayListDomain::SafeDownCast(iter->GetDomain()))
      {
        this->Internal->Domain = iter->GetDomain();
      }
      iter->Next();
    }
    iter->Delete();
  }

  if (this->Internal->Domain)
  {
    this->Internal->Connection->Connect(
      this->Internal->Domain, vtkCommand::DomainModifiedEvent, this, SLOT(domainChanged()));
    this->internalDomainChanged();
  }
}

pqComboBoxDomain::~pqComboBoxDomain()
{
  delete this->Internal;
}

void pqComboBoxDomain::addString(const QString& str)
{
  this->Internal->UserStrings.push_back(str);
  this->domainChanged();
}

void pqComboBoxDomain::insertString(int index, const QString& str)
{
  this->Internal->UserStrings.insert(index, str);
  this->domainChanged();
}

void pqComboBoxDomain::removeString(const QString& str)
{
  int index = this->Internal->UserStrings.indexOf(str);
  if (index >= 0)
  {
    this->Internal->UserStrings.removeAt(index);
    this->domainChanged();
  }
}

void pqComboBoxDomain::removeAllStrings()
{
  this->Internal->UserStrings.clear();
  this->domainChanged();
}

vtkSMProperty* pqComboBoxDomain::getProperty() const
{
  return this->Internal->Property;
}

vtkSMDomain* pqComboBoxDomain::getDomain() const
{
  return this->Internal->Domain;
}

const QStringList& pqComboBoxDomain::getUserStrings() const
{
  return this->Internal->UserStrings;
}

void pqComboBoxDomain::domainChanged()
{
  if (this->Internal->MarkedForUpdate)
  {
    return;
  }

  this->markForUpdate(true);
  this->internalDomainChanged();
}

void pqComboBoxDomain::internalDomainChanged()
{
  QComboBox* combo = qobject_cast<QComboBox*>(this->parent());
  assert(combo != NULL);
  if (!combo)
  {
    return;
  }

  QList<QString> texts;
  QList<QString> infos; // For enumeration domain only
  QList<QIcon> icons;   // For field data domain only
  QList<QVariant> data;

  pqSMAdaptor::PropertyType type;

  // the "data" corresponding to the current property value. This is used as the
  // value to set as the current value if the combo-box didn't have any valid
  // values to begin with. Otherwise, we try to preserve the current selection
  // in the combo-box, if possible.
  QVariant cur_property_value;

  type = pqSMAdaptor::getPropertyType(this->Internal->Property);
  if (type == pqSMAdaptor::ENUMERATION)
  {
    // Use the domain directly if possible
    bool isEnumerationDomain = false;
    if (this->Internal->Domain)
    {
      vtkSMEnumerationDomain* ed = vtkSMEnumerationDomain::SafeDownCast(this->Internal->Domain);
      if (ed)
      {
        isEnumerationDomain = true;
        bool isFieldDataDomain =
          (vtkSMFieldDataDomain::SafeDownCast(this->Internal->Domain) != nullptr);
        for (unsigned int i = 0; i < ed->GetNumberOfEntries(); i++)
        {
          texts.append(ed->GetEntryText(i));
          data.append(ed->GetEntryText(i));
          if (const char* info = ed->GetInfoText(i))
          {
            infos.append(info);
          }
          else
          {
            infos.append(QString());
          }
          icons.append(isFieldDataDomain ? ::get_icon(ed->GetEntryValue(i)) : QIcon());
        }
      }
    }

    if (!isEnumerationDomain)
    {
      QList<QVariant> enums;
      enums = pqSMAdaptor::getEnumerationPropertyDomain(this->Internal->Property);
      for (QVariant var : enums)
      {
        texts.append(var.toString());
        data.append(var.toString());
        infos.append(QString());
        icons.append(QIcon());
      }
    }
    combo->setEnabled(texts.size() > 1);
    cur_property_value = pqSMAdaptor::getEnumerationProperty(this->Internal->Property);
  }
  else if (type == pqSMAdaptor::PROXYSELECTION || type == pqSMAdaptor::PROXYLIST)
  {
    QList<pqSMProxy> proxies = pqSMAdaptor::getProxyPropertyDomain(this->Internal->Property);
    foreach (vtkSMProxy* pxy, proxies)
    {
      texts.append(pxy->GetXMLLabel());
      data.append(pxy->GetXMLLabel());
      infos.append(QString());
      icons.append(QIcon());
    }
    pqSMProxy cur_value = pqSMAdaptor::getProxyProperty(this->Internal->Property);
    if (cur_value)
    {
      cur_property_value = cur_value->GetXMLLabel();
    }
  }

  foreach (QString userStr, this->Internal->UserStrings)
  {
    if (!texts.contains(userStr))
    {
      texts.push_front(userStr);
      data.push_front(userStr);
      infos.append(QString());
      icons.append(QIcon());
    }
  }

  // texts and data must be of the same size.
  assert(texts.size() == data.size());

  // check if the texts didn't change
  QList<QVariant> oldData;

  for (int i = 0; i < combo->count(); i++)
  {
    oldData.append(combo->itemData(i));
  }

  if (oldData != data)
  {
    // save previous value to put back
    QVariant old;
    if (combo->count() > 0)
    {
      old = combo->itemData(combo->currentIndex());
    }
    else
    {
      // the combo-box doesn't have any values currently, which implies that the
      // domain is being setup of the first time. However, it's still possible
      // that the property's value was already set (eg. undo/redo) so we need to
      // ensure that we use the property value rather than the item in the
      // combo-box.
      old = cur_property_value;
    }
    bool prev = combo->blockSignals(true);
    combo->clear();
    for (int cc = 0; cc < data.size(); cc++)
    {
      if (!infos[cc].isEmpty())
      {
        combo->addItem(icons[cc], texts[cc] + " (" + infos[cc] + ")", data[cc]);
      }
      else
      {
        combo->addItem(icons[cc], texts[cc], data[cc]);
      }
    }
    combo->setCurrentIndex(-1);
    combo->blockSignals(prev);
    int foundOld = combo->findData(old);
    if (foundOld >= 0)
    {
      combo->setCurrentIndex(foundOld);
    }
    else
    {
      // Old value was not present, reset to domain default and recover new default
      this->Internal->Property->ResetToDomainDefaults();
      switch (type)
      {
        case (pqSMAdaptor::ENUMERATION):
          cur_property_value = pqSMAdaptor::getEnumerationProperty(this->Internal->Property);
          break;
        case (pqSMAdaptor::PROXYSELECTION):
        case (pqSMAdaptor::PROXYLIST):
        {
          pqSMProxy cur_value = pqSMAdaptor::getProxyProperty(this->Internal->Property);
          if (cur_value)
          {
            cur_property_value = cur_value->GetXMLLabel();
          }
        }
        break;
        default:
          break;
      }
      int foundDefault = combo->findData(cur_property_value);
      if (foundDefault >= 0)
      {
        combo->setCurrentIndex(foundDefault);
      }
      else
      {
        combo->setCurrentIndex(0);
      }
    }
  }
  this->markForUpdate(false);
}

void pqComboBoxDomain::markForUpdate(bool mark)
{
  this->Internal->MarkedForUpdate = mark;
}
