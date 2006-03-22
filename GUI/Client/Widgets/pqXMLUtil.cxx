
#include "pqXMLUtil.h"

#include <QStringList>
#include "vtkPVXMLElement.h"


vtkPVXMLElement *ParaQ::FindNestedElementByName(
    vtkPVXMLElement *element, const char *name)
{
  if(element && name)
    {
    QString qname = name;
    vtkPVXMLElement *child = 0;
    unsigned int total = element->GetNumberOfNestedElements();
    for(unsigned int i = 0; i < total; i++)
      {
      child = element->GetNestedElement(i);
      if(child && qname == child->GetName())
        {
        return child;
        }
      }
    }

  return 0;
}

QString ParaQ::GetStringFromIntList(const QList<int> &list)
{
  QString number;
  QStringList values;
  QList<int>::ConstIterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    number.setNum(*iter);
    values.append(number);
    }

  return values.join(".");
}

QList<int> ParaQ::GetIntListFromString(const char *value)
{
  QList<int> list;
  if(value)
    {
    QStringList values = QString(value).split(".");
    QStringList::Iterator iter = values.begin();
    for( ; iter != values.end(); ++iter)
      {
      list.append((*iter).toInt());
      }
    }

  return list;
}


