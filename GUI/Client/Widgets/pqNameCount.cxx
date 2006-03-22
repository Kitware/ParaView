
/// \file pqNameCount.cxx
/// \date 12/9/2005

#include "pqNameCount.h"

#include <QHash>
#include <QString>


class pqNameCountInternal : public QHash<QString, unsigned int> {};


pqNameCount::pqNameCount()
{
  this->Internal = new pqNameCountInternal();
}

pqNameCount::~pqNameCount()
{
  if(this->Internal)
    delete this->Internal;
}

unsigned int pqNameCount::GetCount(const QString &name)
{
  unsigned int count = 1;
  if(this->Internal)
    {
    QHash<QString, unsigned int>::Iterator iter = this->Internal->find(name);
    if(iter == this->Internal->end())
      this->Internal->insert(name, count);
    else
      count = *iter;
    }

  return count;
}

unsigned int pqNameCount::GetCountAndIncrement(const QString &name)
{
  unsigned int count = 1;
  if(this->Internal)
    {
    QHash<QString, unsigned int>::Iterator iter = this->Internal->find(name);
    if(iter == this->Internal->end())
      this->Internal->insert(name, count + 1);
    else
      {
      count = *iter;
      (*iter) += 1;
      }
    }

  return count;
}

void pqNameCount::IncrementCount(const QString &name)
{
  if(this->Internal)
    {
    QHash<QString, unsigned int>::Iterator iter = this->Internal->find(name);
    if(iter != this->Internal->end())
      (*iter) += 1;
    }
}

void pqNameCount::SetCount(const QString &name, unsigned int count)
{
  if(this->Internal)
    {
    QHash<QString, unsigned int>::Iterator iter = this->Internal->find(name);
    if(iter != this->Internal->end())
      {
      (*iter) = count;
      }
    else
      {
      // Add the new name into the map.
      this->Internal->insert(name, count);
      }
    }
}

void pqNameCount::Reset()
{
  if(this->Internal)
    {
    this->Internal->clear();
    }
}


