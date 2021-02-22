/*=========================================================================

   Program:   ParaView
   Module:    pqXMLUtil.cxx

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
