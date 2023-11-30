// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxyCategory.h"

#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMObject.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#include "pqApplicationCore.h"
#include "pqSettings.h"

#include <QCoreApplication>

#include <sstream>

namespace
{
static QString SETTINGS_CATEGORY_KEY = QStringLiteral("categories.%1/");

QString makeUniqueString(const QString& suggestedString, QStringList list)
{
  int suffix = 0;
  QString uniqueString = suggestedString;
  while (list.contains(uniqueString))
  {
    uniqueString = QString("%1_%2").arg(suggestedString).arg(QString::number(suffix));
    suffix++;
  }

  return uniqueString;
}

}

//-----------------------------------------------------------------------------
pqProxyCategory::pqProxyCategory(pqProxyCategory* parent)
  : pqProxyCategory(parent, "", "")
{
}

//-----------------------------------------------------------------------------
pqProxyCategory::pqProxyCategory(pqProxyCategory* parent, const QString& name, const QString& label)
  : Superclass(parent)
  , Name(name)
  , Label(label)
{
  this->updateLabel(label);

  if (parent)
  {
    parent->addCategory(this);
  }
}

//-----------------------------------------------------------------------------
pqProxyCategory::~pqProxyCategory()
{
  this->clear();
}

//-----------------------------------------------------------------------------
void pqProxyCategory::rename(const QString& name)
{
  if (this->Name == name)
  {
    return;
  }

  this->Name = name;

  // update parent cache.
  auto parent = this->parentCategory();
  if (parent)
  {
    parent->removeCategory(name);
    parent->addCategory(this);
  }
}

//-----------------------------------------------------------------------------
void pqProxyCategory::updateLabel(const QString& label)
{
  this->Label = label;

  if (this->Label.isEmpty())
  {
    this->Label = QString(vtkSMObject::CreatePrettyLabel(this->Name.toStdString()).c_str());
  }
}

//-----------------------------------------------------------------------------
void pqProxyCategory::copyAttributes(pqProxyCategory* other)
{
  this->Name = other->Name;
  this->updateLabel(other->Label);
  this->PreserveOrder = other->PreserveOrder;
  this->ShowInToolbar = other->ShowInToolbar;
}

//-----------------------------------------------------------------------------
void pqProxyCategory::deepCopy(pqProxyCategory* other)
{
  this->copyAttributes(other);
  for (auto proxyName : other->OrderedProxies)
  {
    this->addProxy(new pqProxyInfo(this, other->Proxies[proxyName]));
  }

  for (auto subcategory : other->SubCategories)
  {
    auto newCategory = new pqProxyCategory(this);
    newCategory->deepCopy(subcategory);
    this->addCategory(newCategory);
  }
}

//-----------------------------------------------------------------------------
void pqProxyCategory::addCategory(pqProxyCategory* category)
{
  category->setParent(this);

  for (auto categoryName : this->SubCategories.keys())
  {
    if (this->SubCategories[categoryName] == category)
    {
      this->SubCategories.remove(categoryName);
      break;
    }
  }

  this->SubCategories[category->Name] = category;
}

//-----------------------------------------------------------------------------
void pqProxyCategory::cleanDeletedProxy(QObject* deleted)
{
  auto deletedProxy = dynamic_cast<pqProxyInfo*>(deleted);
  this->Proxies.remove(deletedProxy->name());
  this->OrderedProxies.removeAll(deletedProxy->name());
}

//-----------------------------------------------------------------------------
void pqProxyCategory::clear()
{
  for (auto subCategory : this->SubCategories)
  {
    subCategory->clear();
    subCategory->deleteLater();
  }

  this->SubCategories.clear();
  this->Proxies.clear();
  this->OrderedProxies.clear();
}

//-----------------------------------------------------------------------------
void pqProxyCategory::convertToXML(pqProxyCategory* mainCategory, vtkPVXMLElement* root)
{
  for (auto subCategory : mainCategory->SubCategories)
  {
    vtkNew<vtkPVXMLElement> categoryXML;
    categoryXML->SetName("Category");
    categoryXML->SetAttribute("name", subCategory->Name.toStdString().c_str());
    categoryXML->SetAttribute("menu_label", subCategory->Label.toStdString().c_str());
    if (subCategory->ShowInToolbar)
    {
      categoryXML->SetAttribute("show_in_toolbar", "1");
    }
    if (subCategory->PreserveOrder)
    {
      categoryXML->SetAttribute("preserve_order", "1");
    }

    root->AddNestedElement(categoryXML);
    pqProxyCategory::convertToXML(subCategory, categoryXML);
  }
  for (auto proxyInfo : mainCategory->Proxies)
  {
    if (!proxyInfo)
    {
      continue;
    }

    proxyInfo->convertToXML(root);
  }
}

//-----------------------------------------------------------------------------
void pqProxyCategory::convertLegacyXML(vtkPVXMLElement* root)
{
  if (!root | !root->GetName())
  {
    return;
  }
  if (strcmp(root->GetName(), "Source") == 0)
  {
    root->SetName("Proxy");
    root->AddAttribute("group", "sources");
  }
  else if (strcmp(root->GetName(), "Filter") == 0)
  {
    root->SetName("Proxy");
    root->AddAttribute("group", "filters");
  }
  else if (strcmp(root->GetName(), "Reader") == 0)
  {
    root->SetName("Proxy");
    root->AddAttribute("group", "sources");
  }
  else if (strcmp(root->GetName(), "Writer") == 0)
  {
    root->SetName("Proxy");
    root->AddAttribute("group", "writers");
  }
  for (unsigned int cc = 0; cc < root->GetNumberOfNestedElements(); cc++)
  {
    convertLegacyXML(root->GetNestedElement(cc));
  }
}

//-----------------------------------------------------------------------------
pqProxyCategory* pqProxyCategory::addCategory(const QString& categoryName, vtkPVXMLElement* node)
{
  pqCategoryMap allCategories = this->getSubCategories();

  if (allCategories.contains(categoryName))
  {
    return allCategories[categoryName];
  }

  QString categoryLabel = node->GetAttribute("menu_label");
  int preserve_order = 0;
  node->GetScalarAttribute("preserve_order", &preserve_order);
  int show_in_toolbar = 0;
  node->GetScalarAttribute("show_in_toolbar", &show_in_toolbar);

  pqProxyCategory* category = new pqProxyCategory(this, categoryName, categoryLabel);
  category->PreserveOrder = category->PreserveOrder || (preserve_order == 1);
  category->ShowInToolbar = category->ShowInToolbar || (show_in_toolbar == 1);

  return category;
}

//-----------------------------------------------------------------------------
void pqProxyCategory::removeCategory(const QString& name)
{
  this->SubCategories.remove(name);
}

//-----------------------------------------------------------------------------
void pqProxyCategory::removeProxy(const QString& name)
{
  this->Proxies.remove(name);
  this->OrderedProxies.removeAll(name);
}

//-----------------------------------------------------------------------------
void pqProxyCategory::addProxy(pqProxyInfo* proxyInfo)
{
  if (!proxyInfo || proxyInfo->name().isEmpty() || proxyInfo->group().isEmpty())
  {
    return;
  }

  const QString& proxyName = proxyInfo->name();
  const QString& proxyGroup = proxyInfo->group();

  QString label = proxyInfo->label();
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if (proxyManager)
  {
    vtkSMProxy* prototype =
      proxyManager->GetPrototypeProxy(proxyGroup.toUtf8().data(), proxyName.toUtf8().data());
    if (prototype)
    {
      label = prototype->GetXMLLabel();
    }
  }

  proxyInfo->setLabel(label);

  if (this->Proxies.contains(proxyName))
  {
    proxyInfo->merge(this->Proxies[proxyName]);
  }
  else
  {
    this->OrderedProxies.push_back(proxyName);
  }

  this->Proxies.insert(proxyName, proxyInfo);
  this->connect(proxyInfo, &QObject::destroyed, this, &pqProxyCategory::cleanDeletedProxy,
    Qt::DirectConnection);
}

//-----------------------------------------------------------------------------
bool pqProxyCategory::parseXML(vtkPVXMLElement* root)
{
  return this->parseXML(this, root);
}

//-----------------------------------------------------------------------------
bool pqProxyCategory::parseXML(pqProxyCategory* categoryRoot, vtkPVXMLElement* xmlRoot)
{
  unsigned int numElems = xmlRoot->GetNumberOfNestedElements();

  bool modified = false;
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    vtkPVXMLElement* curElem = xmlRoot->GetNestedElement(cc);
    if (!curElem || !curElem->GetName())
    {
      continue;
    }

    if (strcmp(curElem->GetName(), "Category") == 0 && curElem->GetAttribute("name"))
    {
      QString categoryName = curElem->GetAttribute("name");
      auto category = categoryRoot->addCategory(categoryName, curElem);

      // recurse
      pqProxyCategory::parseXML(category, curElem);
      modified = true;
    }
    else if (strcmp(curElem->GetName(), "Proxy") == 0)
    {
      auto proxyInfo = new pqProxyInfo(categoryRoot, curElem);
      if (proxyInfo->name().isEmpty() || proxyInfo->group().isEmpty())
      {
        proxyInfo->deleteLater();
      }
      else
      {
        categoryRoot->addProxy(proxyInfo);
        modified = true;
      }
    }
  }

  return modified;
}

//-----------------------------------------------------------------------------
bool pqProxyCategory::parseXMLHintsTag(
  const QString& group, const QString& name, vtkPVXMLElement* hints)
{
  if (strcmp(hints->GetName(), "Hints") != 0)
  {
    return false;
  }

  bool modified = false;
  for (unsigned int cc = 0; cc < hints->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* showInMenu = hints->GetNestedElement(cc);
    if (showInMenu == nullptr || showInMenu->GetName() == nullptr ||
      strcmp(showInMenu->GetName(), "ShowInMenu") != 0)
    {
      continue;
    }

    pqProxyCategory* category = this;
    if (const char* categoryName = showInMenu->GetAttribute("category"))
    {
      category = this->addCategory(categoryName, showInMenu);
    }

    category->addProxy(
      new pqProxyInfo(category, name, group, "", showInMenu->GetAttribute("icon"), QStringList()));
    modified = true;
  }

  return modified;
}

//-----------------------------------------------------------------------------
QList<pqProxyInfo*> pqProxyCategory::getProxiesRecursive()
{
  QList<pqProxyInfo*> proxies = this->getRootProxies();

  for (auto category : this->SubCategories)
  {
    proxies << category->getProxiesRecursive();
  }

  return proxies;
}

//-----------------------------------------------------------------------------
bool pqProxyCategory::hasProxy(const QString& name)
{
  return this->Proxies.contains(name);
}

//-----------------------------------------------------------------------------
pqProxyInfo* pqProxyCategory::findProxy(const QString& name, bool recursive)
{
  if (this->Proxies.contains(name))
  {
    return this->Proxies[name];
  }

  if (!recursive)
  {
    return nullptr;
  }

  for (auto category : this->SubCategories)
  {
    auto proxy = category->findProxy(name, true);
    if (proxy)
    {
      return proxy;
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
QList<pqProxyInfo*> pqProxyCategory::getRootProxies()
{
  return this->Proxies.values();
}

//-----------------------------------------------------------------------------
QStringList pqProxyCategory::getOrderedRootProxiesNames()
{
  return this->OrderedProxies;
}

//-----------------------------------------------------------------------------
bool pqProxyCategory::hasProxiesRecursive()
{
  if (!this->getRootProxies().isEmpty())
  {
    return true;
  }

  for (auto category : this->SubCategories)
  {
    if (category->hasProxiesRecursive())
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
QMap<QString, pqProxyCategory*> pqProxyCategory::getSubCategoriesRecursive()
{
  pqCategoryMap categories = this->SubCategories;

  for (auto cat : this->SubCategories)
  {
    auto subCategories = cat->getSubCategoriesRecursive();
    for (auto subCategoryName : subCategories.keys())
    {
      categories.insert(subCategoryName, subCategories[subCategoryName]);
    }
  }

  return categories;
}

//-----------------------------------------------------------------------------
pqProxyCategory* pqProxyCategory::findSubCategory(const QString& name)
{
  if (this->SubCategories.contains(name))
  {
    return this->SubCategories[name];
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
QMap<QString, pqProxyCategory*> pqProxyCategory::getSubCategories()
{
  return this->SubCategories;
}

//-----------------------------------------------------------------------------
QList<pqProxyCategory*> pqProxyCategory::getCategoriesAlphabetically()
{
  QList<pqProxyCategory*> orderedCategories;
  for (const auto& subCategory : this->getSubCategories())
  {
    orderedCategories << subCategory;
  }

  auto nameSort = [&](pqProxyCategory* categoryA, pqProxyCategory* categoryB) {
    QString nameA = categoryA->name().remove("&").toLower();
    QString nameB = categoryB->name().remove("&").toLower();
    return nameA < nameB;
  };

  std::sort(orderedCategories.begin(), orderedCategories.end(), nameSort);

  return orderedCategories;
}

//-----------------------------------------------------------------------------
void pqProxyCategory::writeSettings(const QString& resourceTag)
{
  vtkNew<vtkPVXMLElement> root;
  root->SetName(resourceTag.toStdString().c_str());
  this->convertToXML(root);

  std::stringstream sstream;
  root->PrintXML(sstream, vtkIndent());

  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString value(sstream.str().c_str());
  settings->setValue(::SETTINGS_CATEGORY_KEY.arg(resourceTag), value);
  vtkLogF(TRACE, "Categories written to settings.");
  settings->alertSettingsModified();
}

//-----------------------------------------------------------------------------
void pqProxyCategory::loadSettings(const QString& resourceTag)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (!settings->contains(::SETTINGS_CATEGORY_KEY.arg(resourceTag)))
  {
    return;
  }

  vtkNew<vtkPVXMLParser> parser;
  QString settingValue = settings->value(::SETTINGS_CATEGORY_KEY.arg(resourceTag)).toString();
  if (settingValue.isEmpty())
  {
    return;
  }

  int result = parser->Parse(settingValue.toUtf8().data());
  if (result == 0)
  {
    return;
  }

  this->clear();
  vtkPVXMLElement* root = parser->GetRootElement();
  this->parseXML(root);
  vtkLogF(TRACE, "Categories loaded from settings.");
}

//-----------------------------------------------------------------------------
bool pqProxyCategory::isEmpty()
{
  return this->SubCategories.empty() && this->Proxies.empty();
}

//-----------------------------------------------------------------------------
QString pqProxyCategory::label()
{
  return QCoreApplication::translate("ServerManagerXML", this->Label.toStdString().c_str());
}

//-----------------------------------------------------------------------------
QString pqProxyCategory::makeUniqueCategoryName(const QString& suggestedName)
{
  return ::makeUniqueString(suggestedName, this->SubCategories.keys());
}

//-----------------------------------------------------------------------------
QString pqProxyCategory::makeUniqueCategoryLabel(const QString& suggestedName)
{
  QStringList labels;
  for (auto category : this->SubCategories)
  {
    labels << category->label();
  }

  return ::makeUniqueString(suggestedName, labels);
}

//-----------------------------------------------------------------------------
void pqProxyCategory::setShowInToolbar(bool show)
{
  this->ShowInToolbar = show;
}
