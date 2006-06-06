
/// \file pqSourceInfoGroupMap.cxx
/// \date 5/31/2006

#include "pqSourceInfoGroupMap.h"

#include "pqSourceInfoModel.h"

#include <QList>
#include <QStack>
#include <QString>
#include <QStringList>
#include <QtDebug>

#include "vtkPVXMLElement.h"


class pqSourceInfoGroupMapItem
{
public:
  pqSourceInfoGroupMapItem(pqSourceInfoGroupMapItem *parent=0);
  ~pqSourceInfoGroupMapItem();

  pqSourceInfoGroupMapItem *Parent;
  QList<pqSourceInfoGroupMapItem *> Children;
  QString Name;
  bool IsFolder;
};


class pqSourceInfoGroupMapXml
{
public:
  pqSourceInfoGroupMapXml(vtkPVXMLElement *element=0,
      pqSourceInfoGroupMapItem *item=0);
  ~pqSourceInfoGroupMapXml() {}

  vtkPVXMLElement *Element;
  pqSourceInfoGroupMapItem *Item;
  unsigned int Index;
};


pqSourceInfoGroupMapItem::pqSourceInfoGroupMapItem(
    pqSourceInfoGroupMapItem *parent)
  : Children(), Name()
{
  this->Parent = parent;
  this->IsFolder = false;
}

pqSourceInfoGroupMapItem::~pqSourceInfoGroupMapItem()
{
  QList<pqSourceInfoGroupMapItem *>::Iterator iter = this->Children.begin();
  for( ; iter != this->Children.end(); ++iter)
    {
    delete *iter;
    }

  this->Children.clear();
}


pqSourceInfoGroupMapXml::pqSourceInfoGroupMapXml(vtkPVXMLElement *element,
    pqSourceInfoGroupMapItem *item)
{
  this->Element = element;
  this->Item = item;
  this->Index = 0;
}


pqSourceInfoGroupMap::pqSourceInfoGroupMap(QObject *parentObject)
  : QObject(parentObject)
{
  this->Root = new pqSourceInfoGroupMapItem();
}

pqSourceInfoGroupMap::~pqSourceInfoGroupMap()
{
  if(this->Root)
    {
    delete this->Root;
    }
}

void pqSourceInfoGroupMap::loadSourceInfo(vtkPVXMLElement *root)
{
  // Signal the observers that the data is being cleared. Then, clean
  // up all the grouping items.
  emit this->clearingData();
  if(this->Root)
    {
    delete this->Root;
    }

  // Create the new root item.
  this->Root = new pqSourceInfoGroupMapItem();
  if(!this->Root)
    {
    return;
    }

  if(!root || root->GetNumberOfNestedElements() == 0)
    {
    return;
    }

  // Add the favorites item to the map.
  pqSourceInfoGroupMapItem *item = new pqSourceInfoGroupMapItem(this->Root);
  if(item)
    {
    item->Name = "Favorites";
    item->IsFolder = true;
    this->Root->Children.append(item);
    emit this->groupAdded("Favorites");
    }

  // Read through the xml to create groups and add sources to them.
  QString elementName;
  bool addToStack = false;
  vtkPVXMLElement *element = 0;
  QString attribute;
  QStringList group;
  QStack<pqSourceInfoGroupMapXml *> stack;
  stack.push(new pqSourceInfoGroupMapXml(root, this->Root));
  pqSourceInfoGroupMapXml *current = stack.top();
  while(stack.size() > 0)
    {
    // Get the next nested element from the current xml element.
    addToStack = false;
    pqSourceInfoGroupMapXml *current = stack.top();
    element = current->Element->GetNestedElement(current->Index);
    elementName = element->GetName();
    if(elementName == "Filter" || elementName == "Source")
      {
      // Get the source name from the element.
      attribute = element->GetAttribute("name");
      if(!attribute.isEmpty() && !this->isNameInItem(attribute, current->Item))
        {
        item = new pqSourceInfoGroupMapItem(current->Item);
        if(item)
          {
          item->Name = attribute;
          item->IsFolder = false;
          current->Item->Children.append(item);
          emit this->sourceAdded(item->Name, group.join("/"));
          }
        }
      }
    else if(elementName == "Category")
      {
      // Get the group name from the element. Ignore the 'Alphabetical'
      // category in the old filter menu xml.
      attribute = element->GetAttribute("name");
      if(!attribute.isEmpty() && attribute != "Alphabetical")
        {
        // See if the group already exists. Make sure there are no '/'
        // characters in the category name (CHT/AMR from the old xml).
        attribute.replace("/", "-");
        group.append(attribute);
        item = this->getChildItem(current->Item, attribute);
        if(!item)
          {
          item = new pqSourceInfoGroupMapItem(current->Item);
          if(item)
            {
            item->Name = attribute;
            item->IsFolder = true;
            current->Item->Children.append(item);
            emit this->groupAdded(group.join("/"));
            }
          }

        if(item && element->GetNumberOfNestedElements() > 0)
          {
          addToStack = true;
          }
        else
          {
          // If the group is not being added to the stack, remove the
          // name from the group name stack.
          group.removeLast();
          }
        }
      }
    else if(elementName == "CategoryGroup")
      {
      // Support the old xml format for the filter menu. The current
      // item will be the same. The catagory group element needs to
      // be pushed on the stack.
      addToStack = true;
      item = current->Item;
      }

    if(addToStack)
      {
      stack.push(new pqSourceInfoGroupMapXml(element, item));
      current = stack.top();
      }
    else
      {
      // When the index reaches the end of the nested elements, the
      // stack item needs to be removed. Increment the index of the
      // next stack item to keep the reader moving.
      while(++current->Index >= current->Element->GetNumberOfNestedElements())
        {
        delete current;
        current = 0;
        stack.pop();
        if(stack.size() == 0)
          {
          break;
          }

        current = stack.top();
        if(group.size() > 0)
          {
          group.removeLast();
          }
        }
      }
    }
}

void pqSourceInfoGroupMap::saveSourceInfo(vtkPVXMLElement *)
{
}

void pqSourceInfoGroupMap::addGroup(const QString &group)
{
  if(group.isEmpty())
    {
    qDebug() << "Unable to add empty group to the source info map.";
    return;
    }

  // Split the group path in order to get the parent group.
  QStringList paths = group.split("/", QString::SkipEmptyParts);
  QString groupName = paths.takeLast();
  pqSourceInfoGroupMapItem *parentItem = this->Root;
  if(paths.size() > 0)
    {
    QString groupPath = paths.join("/");
    parentItem = this->getGroupItemFor(groupPath);
    }

  if(!parentItem)
    {
    qDebug() << "Group's parent path not found in the source info map.";
    return;
    }

  // Make sure the parent group does not already have the sub-group.
  if(this->isNameInItem(groupName, parentItem))
    {
    qDebug() << "Group already exists in source info map.";
    return;
    }

  // Create a new model item for the group.
  pqSourceInfoGroupMapItem *groupItem = new pqSourceInfoGroupMapItem(parentItem);
  if(groupItem)
    {
    groupItem->Name = groupName;
    groupItem->IsFolder = true;
    parentItem->Children.append(groupItem);
    emit this->groupAdded(group);
    }
}

void pqSourceInfoGroupMap::removeGroup(const QString &group)
{
  if(group.isEmpty())
    {
    qDebug() << "Unable to remove empty group from the source info map.";
    return;
    }

  pqSourceInfoGroupMapItem *groupItem = this->getGroupItemFor(group);
  if(groupItem)
    {
    if(groupItem->Parent == this->Root && groupItem->Name == "Favorites")
      {
      qDebug() << "Unable to remove \"Favorites\" group.";
      }
    else
      {
      emit this->removingGroup(group);
      groupItem->Parent->Children.removeAll(groupItem);
      delete groupItem;
      }
    }
  else
    {
    qDebug() << "Specified group not found in the source info map.";
    }
}

void pqSourceInfoGroupMap::addSource(const QString &name, const QString &group)
{
  if(name.isEmpty())
    {
    qDebug() << "Unable to add empty source to source info map.";
    return;
    }

  pqSourceInfoGroupMapItem *parentItem = this->getGroupItemFor(group);
  if(!parentItem)
    {
    qDebug() << "Source's parent path not found in the source info map.";
    return;
    }

  // Make sure the parent group does not already have the source.
  if(this->isNameInItem(name, parentItem))
    {
    qDebug() << "Source already exists in the specified group.";
    return;
    }

  // Create a new model item for the group.
  pqSourceInfoGroupMapItem *source = new pqSourceInfoGroupMapItem(parentItem);
  if(source)
    {
    source->Name = name;
    source->IsFolder = false;
    parentItem->Children.append(source);
    emit this->sourceAdded(name, group);
    }
}

void pqSourceInfoGroupMap::removeSource(const QString &name,
    const QString &group)
{
  if(name.isEmpty())
    {
    qDebug() << "Unable to remove empty source from source info map.";
    return;
    }

  pqSourceInfoGroupMapItem *parentItem = this->getGroupItemFor(group);
  if(!parentItem)
    {
    qDebug() << "Source's parent path not found in the source info map.";
    return;
    }

  // Find the source in the parent item.
  pqSourceInfoGroupMapItem *source = this->getChildItem(parentItem, name);
  if(source)
    {
    emit this->removingSource(name, group);
    parentItem->Children.removeAll(source);
    delete source;
    }
  else
    {
    qDebug() << "Source not found in specified group.";
    }
}

void pqSourceInfoGroupMap::initializeModel(pqSourceInfoModel *model) const
{
  if(model)
    {
    // Step through the items in the tree. Add all the groups and
    // sources to the model. The model will filter out any sources
    // that are not available in the model.
    QString group;
    pqSourceInfoGroupMapItem *item = this->getNextItem(this->Root);
    while(item)
      {
      if(item->IsFolder)
        {
        this->getGroupPath(item, group);
        model->addGroup(group);
        }
      else
        {
        this->getGroupPath(item->Parent, group);
        model->addSource(item->Name, group);
        }

      item = this->getNextItem(item);
      }
    }
}

pqSourceInfoGroupMapItem *pqSourceInfoGroupMap::getNextItem(
    pqSourceInfoGroupMapItem *item) const
{
  if(!item)
    {
    return 0;
    }

  if(item->Children.size() > 0)
    {
    return item->Children[0];
    }

  int row = 0;
  while(item->Parent)
    {
    row = item->Parent->Children.indexOf(item);
    if(row < item->Parent->Children.size() - 1)
      {
      return item->Parent->Children[row + 1];
      }

    item = item->Parent;
    }

  return 0;
}

void pqSourceInfoGroupMap::getGroupPath(pqSourceInfoGroupMapItem *item,
    QString &group) const
{
  QStringList paths;
  while(item && item != this->Root)
    {
    paths.prepend(item->Name);
    item = item->Parent;
    }

  if(paths.size() > 0)
    {
    group = paths.join("/");
    }
  else
    {
    group = QString();
    }
}

pqSourceInfoGroupMapItem *pqSourceInfoGroupMap::getGroupItemFor(
    const QString &group) const
{
  if(group.isEmpty())
    {
    return this->Root;
    }

  pqSourceInfoGroupMapItem *item = this->Root;
  QStringList paths = group.split("/", QString::SkipEmptyParts);
  QStringList::Iterator iter = paths.begin();
  for( ; item && iter != paths.end(); ++iter)
    {
    item = this->getChildItem(item, *iter);
    }

  return item;
}

pqSourceInfoGroupMapItem *pqSourceInfoGroupMap::getChildItem(
    pqSourceInfoGroupMapItem *item, const QString &name) const
{
  QList<pqSourceInfoGroupMapItem *>::Iterator iter = item->Children.begin();
  for( ; iter != item->Children.end(); ++iter)
    {
    if((*iter)->Name == name)
      {
      return *iter;
      }
    }

  return 0;
}

bool pqSourceInfoGroupMap::isNameInItem(const QString &name,
    pqSourceInfoGroupMapItem *item) const
{
  return this->getChildItem(item, name) != 0;
}


