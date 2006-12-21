/*=========================================================================

   Program: ParaView
   Module:    pqSourceProxyInfo.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqSourceProxyInfo.cxx
///
/// \date 1/27/2006

#include "pqSourceProxyInfo.h"

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

#include "vtkPVXMLElement.h"


class pqSourceProxyInfoItem
{
public:
  pqSourceProxyInfoItem();
  ~pqSourceProxyInfoItem() {}

  QString Name;
  QString Document;
};


class pqSourceProxyInfoCategory
{
public:
  pqSourceProxyInfoCategory();
  ~pqSourceProxyInfoCategory() {}

  QString Name;
  QString MenuName;
  QList<pqSourceProxyInfoItem *> List;
};


class pqSourceProxyInfoCategoryGroup
{
public:
  pqSourceProxyInfoCategoryGroup();
  ~pqSourceProxyInfoCategoryGroup();

  QList<pqSourceProxyInfoCategory *> List;
};

class pqSourceProxyInfoInternal
{
public:
  pqSourceProxyInfoInternal();
  ~pqSourceProxyInfoInternal() {}

public:
  QList<pqSourceProxyInfoCategoryGroup *> Filters;
  QMap<QString, pqSourceProxyInfoItem *> FilterMap;
};


pqSourceProxyInfoItem::pqSourceProxyInfoItem()
  : Name(), Document()
{
}


pqSourceProxyInfoCategory::pqSourceProxyInfoCategory()
  : Name(), MenuName(), List()
{
}


pqSourceProxyInfoCategoryGroup::pqSourceProxyInfoCategoryGroup()
  : List()
{
}

pqSourceProxyInfoCategoryGroup::~pqSourceProxyInfoCategoryGroup()
{
  // Clean up the categories on the list.
  QList<pqSourceProxyInfoCategory *>::Iterator iter = this->List.begin();
  for( ; iter != this->List.end(); ++iter)
    {
    delete *iter;
    }
}


pqSourceProxyInfoInternal::pqSourceProxyInfoInternal()
  : Filters(), FilterMap()
{
}


pqSourceProxyInfo::pqSourceProxyInfo()
{
  this->Internal = new pqSourceProxyInfoInternal();
}

pqSourceProxyInfo::~pqSourceProxyInfo()
{
  if(this->Internal)
    {
    this->Reset();
    delete this->Internal;
    }
}

void pqSourceProxyInfo::Reset()
{
  if(this->Internal)
    {
    // Clean up the filter information.
    QList<pqSourceProxyInfoCategoryGroup *>::Iterator iter =
        this->Internal->Filters.begin();
    for( ; iter != this->Internal->Filters.end(); ++iter)
      {
      delete *iter;
      }

    QMap<QString, pqSourceProxyInfoItem *>::Iterator jter =
        this->Internal->FilterMap.begin();
    for( ; jter != this->Internal->FilterMap.end(); ++jter)
      {
      delete *jter;
      }

    this->Internal->Filters.clear();
    this->Internal->FilterMap.clear();
    }
}

bool pqSourceProxyInfo::IsFilterInfoLoaded() const
{
  if(this->Internal)
    {
    return this->Internal->Filters.size() > 0;
    }

  return false;
}

void pqSourceProxyInfo::LoadFilterInfo(vtkPVXMLElement *root)
{
  if(!root || !this->Internal)
    {
    return;
    }

  // Make sure the current information gets cleared out.
  this->Reset();

  // Walk through the xml to create the filter information.
  QString name;
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int k = 0;
  const char *attribute = 0;
  vtkPVXMLElement *element = 0;
  vtkPVXMLElement *groupElement = 0;
  vtkPVXMLElement *categoryElement = 0;
  pqSourceProxyInfoItem *filter = 0;
  pqSourceProxyInfoCategoryGroup *group = 0;
  pqSourceProxyInfoCategory *category = 0;
  QMap<QString, pqSourceProxyInfoItem *>::Iterator iter;
  for(i = 0; i < root->GetNumberOfNestedElements(); i++)
    {
    groupElement = root->GetNestedElement(i);
    name = groupElement->GetName();
    if(name != "CategoryGroup")
      {
      continue;
      }

    // Create a category group for the element.
    group = new pqSourceProxyInfoCategoryGroup();
    if(!group)
      {
      continue;
      }

    // Add the group to the list. Get the list of group categories
    // from the elements in the group element.
    this->Internal->Filters.append(group);
    for(j = 0; j < groupElement->GetNumberOfNestedElements(); j++)
      {
      categoryElement = groupElement->GetNestedElement(j);
      name = categoryElement->GetName();
      if(name != "Category")
        {
        continue;
        }

      // Create a category for the element.
      category = new pqSourceProxyInfoCategory();
      if(!category)
        {
        continue;
        }

      // Add the category to the group. Get the category name(s)
      // from the element.
      group->List.append(category);
      attribute = categoryElement->GetAttribute("name");
      if(attribute)
        {
        category->Name = attribute;
        }

      attribute = categoryElement->GetAttribute("menuName");
      if(attribute)
        {
        category->MenuName = attribute;
        }

      // Get the list of filters from the elements in the category
      // elements.
      for(k = 0; k < categoryElement->GetNumberOfNestedElements(); k++)
        {
        element = categoryElement->GetNestedElement(k);
        name = element->GetName();
        if(name != "Filter")
          {
          continue;
          }

        // See if the filter already exists. If not, create a filter
        // item for the element.
        attribute = element->GetAttribute("name");
        if(!attribute)
          {
          continue;
          }

        iter = this->Internal->FilterMap.find(attribute);
        if(iter == this->Internal->FilterMap.end())
          {
          filter = new pqSourceProxyInfoItem();
          if(filter)
            {
            filter->Name = attribute;
            this->Internal->FilterMap.insert(filter->Name, filter);
            }
          }
        else
          {
          filter = *iter;
          }

        if(filter)
          {
          // Add the filter to the category.
          category->List.append(filter);
          }
        }
      }
    }
}

void pqSourceProxyInfo::GetFilterMenu(QStringList &menuList) const
{
  if(!this->Internal)
    {
    return;
    }

  // Add in the menu name for all the categories. If the menu
  // name is empty, use the category name. Put empty strings in
  // the list to separate the category groups.
  QList<pqSourceProxyInfoCategoryGroup *>::Iterator iter =
      this->Internal->Filters.begin();
  for( ; iter != this->Internal->Filters.end(); ++iter)
    {
    if(iter != this->Internal->Filters.begin() && (*iter)->List.size() > 0)
      {
      menuList.append(QString());
      }

    QList<pqSourceProxyInfoCategory *>::Iterator jter = (*iter)->List.begin();
    for( ; jter != (*iter)->List.end(); ++jter)
      {
      if((*jter)->MenuName.isEmpty())
        {
        menuList.append((*jter)->Name);
        }
      else
        {
        menuList.append((*jter)->MenuName);
        }
      }
    }
}

void pqSourceProxyInfo::GetFilterCategories(const QString &name,
    QStringList &list) const
{
  if(!this->Internal)
    {
    return;
    }

  // Get the filter info object from the filter map.
  QMap<QString, pqSourceProxyInfoItem *>::Iterator iter =
      this->Internal->FilterMap.find(name);
  if(iter == this->Internal->FilterMap.end())
    {
    return;
    }

  // Search through the categories for the filter.
  pqSourceProxyInfoItem *filter = *iter;
  QList<pqSourceProxyInfoCategoryGroup *>::Iterator jter =
      this->Internal->Filters.begin();
  for( ; jter != this->Internal->Filters.end(); ++jter)
    {
    QList<pqSourceProxyInfoCategory *>::Iterator kter = (*jter)->List.begin();
    for( ; kter != (*jter)->List.end(); ++kter)
      {
      if((*kter)->List.indexOf(filter) != -1)
        {
        list.append((*kter)->Name);
        }
      }
    }
}

void pqSourceProxyInfo::GetFilterMenuCategories(const QString &name,
    QStringList &list) const
{
  if(!this->Internal)
    {
    return;
    }

  // Get the filter info object from the filter map.
  QMap<QString, pqSourceProxyInfoItem *>::Iterator iter =
      this->Internal->FilterMap.find(name);
  if(iter == this->Internal->FilterMap.end())
    {
    return;
    }

  // Search through the categories for the filter.
  pqSourceProxyInfoItem *filter = *iter;
  QList<pqSourceProxyInfoCategoryGroup *>::Iterator jter =
      this->Internal->Filters.begin();
  for( ; jter != this->Internal->Filters.end(); ++jter)
    {
    QList<pqSourceProxyInfoCategory *>::Iterator kter = (*jter)->List.begin();
    for( ; kter != (*jter)->List.end(); ++kter)
      {
      if((*kter)->List.indexOf(filter) != -1)
        {
        if((*kter)->MenuName.isEmpty())
          {
          list.append((*kter)->Name);
          }
        else
          {
          list.append((*kter)->MenuName);
          }
        }
      }
    }
}


