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
#include <vtkEventQtSlotConnect.h>
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

#include <assert.h>

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
  QString DomainName;
  QStringList UserStrings;
  bool MarkedForUpdate;
};

pqComboBoxDomain::pqComboBoxDomain(QComboBox* p, vtkSMProperty* prop, const QString& domainName)
  : QObject(p)
{
  this->Internal = new pqInternal();
  this->Internal->Property = prop;
  this->Internal->DomainName = domainName;

  if (!domainName.isEmpty())
  {
    this->Internal->Domain = prop->GetDomain(domainName.toLocal8Bit().data());
  }
  else
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

const QString& pqComboBoxDomain::getDomainName() const
{
  return this->Internal->DomainName;
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
  Q_ASSERT(combo != NULL);
  if (!combo)
  {
    return;
  }

  QList<QString> texts;
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
    QList<QVariant> enums;
    enums = pqSMAdaptor::getEnumerationPropertyDomain(this->Internal->Property);
    foreach (QVariant var, enums)
    {
      texts.append(var.toString());
      data.append(var.toString());
    }
    combo->setEnabled(enums.size() > 1);
    cur_property_value = pqSMAdaptor::getEnumerationProperty(this->Internal->Property);
  }
  else if (type == pqSMAdaptor::FIELD_SELECTION)
  {
    if (this->Internal->DomainName == "field_list")
    {
      texts = pqSMAdaptor::getFieldSelectionModeDomain(this->Internal->Property);
      foreach (QString str, texts)
      {
        data.push_back(str);
      }
    }
    else if (this->Internal->DomainName == "array_list")
    {
      QList<QPair<QString, bool> > arrays =
        pqSMAdaptor::getFieldSelectionScalarDomainWithPartialArrays(this->Internal->Property);
      for (int kk = 0; kk < arrays.size(); kk++)
      {
        QPair<QString, bool> pair = arrays[kk];
        QString arrayName = pair.first;
        if (pair.second)
        {
          arrayName += " (partial)";
        }
        texts.append(arrayName);
        data.append(pair.first);
      }
    }
    cur_property_value = pqSMAdaptor::getElementProperty(this->Internal->Property);
  }
  else if (type == pqSMAdaptor::PROXYSELECTION || type == pqSMAdaptor::PROXYLIST)
  {
    QList<pqSMProxy> proxies = pqSMAdaptor::getProxyPropertyDomain(this->Internal->Property);
    foreach (vtkSMProxy* pxy, proxies)
    {
      texts.append(pxy->GetXMLLabel());
      data.append(pxy->GetXMLLabel());
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
      combo->addItem(texts[cc], data[cc]);
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
      combo->setCurrentIndex(0);
    }
  }
  this->markForUpdate(false);
}

void pqComboBoxDomain::markForUpdate(bool mark)
{
  this->Internal->MarkedForUpdate = mark;
}
