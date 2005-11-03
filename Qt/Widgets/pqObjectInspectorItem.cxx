
#include "pqObjectInspectorItem.h"

#include <QVariant>


class pqObjectInspectorItemInternal : public QList<pqObjectInspectorItem *> {};


pqObjectInspectorItem::pqObjectInspectorItem(QObject *parent)
  : QObject(parent), Name(), Value()
{
  this->Internal = 0;
  this->Parent = 0;
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

void pqObjectInspectorItem::setValue(const QVariant &value)
{
  if(this->Value != value)
    {
    this->Value = value;
    emit valueChanged(this);
    }
}

int pqObjectInspectorItem::getChildCount() const
{
  if(this->Internal)
    return this->Internal->size();
  return 0;
}

int pqObjectInspectorItem::getChildIndex(pqObjectInspectorItem *child) const
{
  for(int row = 0; this->Internal && row < this->Internal->size(); row++)
    {
    if((*this->Internal)[row] == child)
      return row;
    }

  return -1;
}

pqObjectInspectorItem *pqObjectInspectorItem::getChild(int index) const
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

void pqObjectInspectorItem::addChild(pqObjectInspectorItem *child)
{
  if(!this->Internal)
    this->Internal = new pqObjectInspectorItemInternal();

  if(this->Internal)
    this->Internal->append(child);
}


