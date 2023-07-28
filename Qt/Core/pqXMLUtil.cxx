// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqXMLUtil.h"

#include "vtkPVXMLElement.h"
#include <QString>
#include <QStringList>

vtkPVXMLElement* pqXMLUtil::FindNestedElementByName(vtkPVXMLElement* element, const char* name)
{
  if (element && name)
  {
    QString qname = name;
    vtkPVXMLElement* child = nullptr;
    unsigned int total = element->GetNumberOfNestedElements();
    for (unsigned int i = 0; i < total; i++)
    {
      child = element->GetNestedElement(i);
      if (child && qname == child->GetName())
      {
        return child;
      }
    }
  }

  return nullptr;
}

QString pqXMLUtil::GetStringFromIntList(const QList<int>& list)
{
  QString number;
  QStringList values;
  QList<int>::ConstIterator iter = list.begin();
  for (; iter != list.end(); ++iter)
  {
    number.setNum(*iter);
    values.append(number);
  }

  return values.join(".");
}

QList<int> pqXMLUtil::GetIntListFromString(const char* value)
{
  QList<int> list;
  if (value)
  {
    QStringList values = QString(value).split(".");
    QStringList::Iterator iter = values.begin();
    for (; iter != values.end(); ++iter)
    {
      list.append((*iter).toInt());
    }
  }

  return list;
}
