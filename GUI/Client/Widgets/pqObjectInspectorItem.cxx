/*=========================================================================

   Program:   ParaQ
   Module:    pqObjectInspectorItem.cxx

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

#include "pqObjectInspectorItem.h"

#include "vtkSMProperty.h"
#include "vtkSMPropertyAdaptor.h"
#include "pqSMAdaptor.h"

#include <QList>
#include <QStringList>


class pqObjectInspectorItemInternal : public QList<pqObjectInspectorItem *> {};


pqObjectInspectorItem::pqObjectInspectorItem(QObject *p)
  : QObject(p), Name(), Value(), Domain()
{
  this->Internal = 0;
  this->Parent = 0;
  this->Modified = false;
}

pqObjectInspectorItem::~pqObjectInspectorItem()
{
  if(this->Internal)
    {
    this->clearChildren();
    delete this->Internal;
    }
}

void pqObjectInspectorItem::setPropertyName(const QString &name)
{
  if(this->Name != name)
    {
    this->Name = name;
    emit nameChanged(this);
    }
}

void pqObjectInspectorItem::setValue(const QVariant &val)
{
  if(this->Value != val)
    {
    this->Value = val;
    emit valueChanged(this);
    }
}

int pqObjectInspectorItem::childCount() const
{
  if(this->Internal)
    return this->Internal->size();
  return 0;
}

int pqObjectInspectorItem::childIndex(pqObjectInspectorItem *c) const
{
  for(int row = 0; this->Internal && row < this->Internal->size(); row++)
    {
    if((*this->Internal)[row] == c)
      return row;
    }

  return -1;
}

pqObjectInspectorItem *pqObjectInspectorItem::child(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->size())
    return (*this->Internal)[index];
  return 0;
}

void pqObjectInspectorItem::clearChildren()
{
  if(this->Internal)
    {
    // Clean up the child list.
    pqObjectInspectorItemInternal::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if(*iter)
        {
        delete *iter;
        *iter = 0;
        }
      }

    this->Internal->clear();
    }
}

void pqObjectInspectorItem::addChild(pqObjectInspectorItem *c)
{
  if(!this->Internal)
    this->Internal = new pqObjectInspectorItemInternal();

  if(this->Internal)
    this->Internal->append(c);
}

void pqObjectInspectorItem::updateDomain(vtkSMProxy* proxy, vtkSMProperty *prop)
{
  if(!prop)
    return;

  if(pqSMAdaptor::instance()->getPropertyType(prop) == pqSMAdaptor::PROXY)
    {
    QList<pqSMProxy> d = pqSMAdaptor::instance()->getProxyPropertyDomain(proxy, prop);
    QList<QVariant> list;
    foreach(pqSMProxy p, d)
      {
      QVariant v;
      v.setValue(p);
      list.append(v);
      }
    this->Domain = list;
    return;
    }

  vtkSMPropertyAdaptor *adaptor = vtkSMPropertyAdaptor::New();
  if(adaptor)
    {
    adaptor->SetProperty(prop);
    if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
      this->Domain = QVariant(QString("StringListRange"));
    else if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::RANGE)
      {
      // Make a list of two values: min, max. If either is missing use
      // an invalid variant as a placeholder.
      int elemType = adaptor->GetElementType();
      QList<QVariant> list;
      if(this->childCount() > 0)
        {
        unsigned int total = adaptor->GetNumberOfRangeElements();
        for(int i = 0; i < this->Internal->size() && i < (int)total; i++)
          {
          list.clear();
          list.append(this->convertLimit(adaptor->GetRangeMinimum(i), elemType));
          list.append(this->convertLimit(adaptor->GetRangeMaximum(i), elemType));
          (*this->Internal)[i]->setDomain(list);
          }
        }
      else
        {
        list.append(this->convertLimit(adaptor->GetRangeMinimum(0), elemType));
        list.append(this->convertLimit(adaptor->GetRangeMaximum(0), elemType));
        this->Domain = list;
        }
      }
    else if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::ENUMERATION)
      {
      if(adaptor->GetElementType() == vtkSMPropertyAdaptor::BOOLEAN)
        this->Domain = QVariant(QString("Boolean"));
      else if(adaptor->GetElementType() == vtkSMPropertyAdaptor::STRING)
        this->Domain = QVariant(QString("StringList"));
      else if(adaptor->GetElementType() == vtkSMPropertyAdaptor::INT)
        {
        // Get the list of enumeration names.
        QStringList names;
        unsigned int total = adaptor->GetNumberOfEnumerationElements();
        for(unsigned int i = 0; i < total; i++)
          names.append(QString(adaptor->GetEnumerationName(i)));
        this->Domain = QVariant(names);
        }
      }

    adaptor->Delete();
    }
}

QVariant pqObjectInspectorItem::convertLimit(const char *limit, int type) const
{
  if(limit)
    {
    QString val = limit;
    if(type == vtkSMPropertyAdaptor::INT)
      return QVariant(val.toInt());
    else if(type == vtkSMPropertyAdaptor::DOUBLE)
      return QVariant(val.toDouble());
    }

  return QVariant();
}


