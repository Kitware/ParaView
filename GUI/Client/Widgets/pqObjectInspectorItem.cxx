
/// \file pqObjectInspectorItem.cxx
/// \brief
///   The pqObjectInspectorItem class is used to represent an object
///   property.
///
/// \date 11/7/2005

#include "pqObjectInspectorItem.h"

#include "vtkSMProperty.h"
#include "vtkSMPropertyAdaptor.h"

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

void pqObjectInspectorItem::updateDomain(vtkSMProperty *prop)
{
  if(!prop)
    return;

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


