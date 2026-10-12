// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxyCategory.h"

#include "vtkNew.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMObject.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include "pqApplicationCore.h"
#include "pqSettings.h"

#include <QCoreApplication>
#include <QSet>
#include <QtDebug>

#include <cstring>
#include <sstream>
#include <tuple>

namespace
{
QString SETTINGS_CATEGORY_KEY = QStringLiteral("categories.%1/");
QString SETTINGS_CUSTOMIZATIONS_KEY = QStringLiteral("categoryCustomizations.%1/");

const char* itemElementName(pqProxyInfo* item)
{
  return item->isScript() ? "Script" : (item->isSeparator() ? "Separator" : "Proxy");
}

bool isItemElement(vtkPVXMLElement* element)
{
  const char* name = element->GetName();
  return name &&
    (strcmp(name, "Proxy") == 0 || strcmp(name, "Script") == 0 || strcmp(name, "Separator") == 0);
}

bool isCategoryElement(vtkPVXMLElement* element)
{
  return element->GetName() && strcmp(element->GetName(), "Category") == 0;
}

/**
 * Returns true if `xml` has a nested element for the same category or item as
 * `element`, i.e. of the same kind with the same name.
 */
bool hasElementFor(vtkPVXMLElement* xml, vtkPVXMLElement* element)
{
  const char* name = element->GetAttribute("name");
  for (unsigned int cc = 0; name && cc < xml->GetNumberOfNestedElements(); ++cc)
  {
    vtkPVXMLElement* nested = xml->GetNestedElement(cc);
    const char* nestedName = nested->GetAttribute("name");
    if (nestedName && strcmp(nestedName, name) == 0 &&
      ::isCategoryElement(nested) == ::isCategoryElement(element))
    {
      return true;
    }
  }
  return false;
}

/**
 * Returns the label of categories without one, derived from their name.
 */
QString defaultLabel(const QString& name)
{
  return QString::fromStdString(vtkSMObject::CreatePrettyLabel(name.toStdString()));
}

/**
 * Parse the XML saved in the settings for the given key, if any.
 */
vtkSmartPointer<vtkPVXMLElement> parseSettings(const QString& key)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  const QString value = settings->contains(key) ? settings->value(key).toString() : QString();
  vtkNew<vtkPVXMLParser> parser;
  if (value.isEmpty() || parser->Parse(value.toUtf8().data()) == 0)
  {
    return nullptr;
  }
  return parser->GetRootElement();
}

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
    this->Label = ::defaultLabel(this->Name);
  }
}

//-----------------------------------------------------------------------------
void pqProxyCategory::copyAttributes(pqProxyCategory* other)
{
  this->Name = other->Name;
  this->updateLabel(other->Label);
  this->PreserveOrder = other->PreserveOrder;
  this->ShowInToolbar = other->ShowInToolbar;
  this->ShowInMenuBar = other->ShowInMenuBar;
}

//-----------------------------------------------------------------------------
void pqProxyCategory::deepCopy(pqProxyCategory* other)
{
  this->copyAttributes(other);
  for (const auto& proxyName : other->OrderedProxies)
  {
    this->addProxy(new pqProxyInfo(this, other->Proxies[proxyName]));
  }

  for (auto subcategory : other->SubCategories)
  {
    auto newCategory = new pqProxyCategory(this);
    newCategory->deepCopy(subcategory);
    this->addCategory(newCategory);
  }

  // e.g. the copy edited using the Configure Categories dialog keeps the
  // customizations that could not be applied, when saved.
  if (other->UnappliedCustomizations)
  {
    this->UnappliedCustomizations = vtkSmartPointer<vtkPVXMLElement>::New();
    other->UnappliedCustomizations->CopyTo(this->UnappliedCustomizations);
  }
}

//-----------------------------------------------------------------------------
void pqProxyCategory::addCategory(pqProxyCategory* category)
{
  category->setParent(this);

  for (const auto& categoryName : this->SubCategories.keys())
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
  this->UnappliedCustomizations = nullptr;
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
    if (subCategory->ShowInMenuBar)
    {
      categoryXML->SetAttribute("show_in_menubar", "1");
    }

    root->AddNestedElement(categoryXML);
    pqProxyCategory::convertToXML(subCategory, categoryXML);
  }
  for (const auto& proxyName : mainCategory->OrderedProxies)
  {
    auto proxyInfo = mainCategory->Proxies.value(proxyName);
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
  int preserve_order = 0;
  node->GetScalarAttribute("preserve_order", &preserve_order);
  int show_in_toolbar = 0;
  node->GetScalarAttribute("show_in_toolbar", &show_in_toolbar);
  int show_in_menubar = 0;
  node->GetScalarAttribute("show_in_menubar", &show_in_menubar);

  // several configurations, e.g. from plugins, can add items to the same
  // category. Flags are enabled if any of them requests it.
  pqProxyCategory* category = this->findSubCategory(categoryName);
  if (!category)
  {
    category = new pqProxyCategory(this, categoryName, node->GetAttribute("menu_label"));
  }
  else if (node->GetAttribute("menu_label") && category->Label == ::defaultLabel(categoryName))
  {
    // the category was created without a label, e.g. by a configuration only
    // adding items to it: use the one given, so that the label does not depend
    // on the order configurations are loaded in.
    category->updateLabel(node->GetAttribute("menu_label"));
  }
  category->PreserveOrder = category->PreserveOrder || (preserve_order == 1);
  category->ShowInToolbar = category->ShowInToolbar || (show_in_toolbar == 1);
  category->ShowInMenuBar = category->ShowInMenuBar || (show_in_menubar == 1);

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
  if (!proxyInfo || !proxyInfo->isValid())
  {
    return;
  }

  const QString& proxyName = proxyInfo->name();
  const QString& proxyGroup = proxyInfo->group();

  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if (proxyInfo->isProxy() && proxyManager)
  {
    vtkSMProxy* prototype =
      proxyManager->GetPrototypeProxy(proxyGroup.toUtf8().data(), proxyName.toUtf8().data());
    if (prototype)
    {
      proxyInfo->setLabel(prototype->GetXMLLabel());
    }
  }

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
    else if (strcmp(curElem->GetName(), "Proxy") == 0 ||
      strcmp(curElem->GetName(), "Script") == 0 || strcmp(curElem->GetName(), "Separator") == 0)
    {
      auto proxyInfo = new pqProxyInfo(categoryRoot, curElem);
      if (proxyInfo->isSeparator() && proxyInfo->name().isEmpty())
      {
        // separators are identified by their position among the separators of
        // the category, unique even when merging several configurations.
        proxyInfo->setName(
          ::makeUniqueString(QStringLiteral("Separator%1").arg(categoryRoot->separatorCount() + 1),
            categoryRoot->OrderedProxies));
      }
      if (!proxyInfo->isValid())
      {
        proxyInfo->deleteLater();
      }
      else
      {
        if (proxyInfo->isScript())
        {
          pqProxyCategory::warnIfScriptNameIsUsed(categoryRoot, proxyInfo);
        }
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

    // definitions are looked for again, e.g. whenever plugins are loaded: only
    // report actual changes, as the menus are rebuilt then.
    pqProxyCategory* category = this;
    if (const char* categoryName = showInMenu->GetAttribute("category"))
    {
      auto attributes = [](pqProxyCategory* cat) {
        return std::make_tuple(
          cat->Label, cat->PreserveOrder, cat->ShowInToolbar, cat->ShowInMenuBar);
      };
      pqProxyCategory* existing = this->findSubCategory(categoryName);
      const auto previousAttributes =
        existing ? attributes(existing) : decltype(attributes(this))();
      category = this->addCategory(categoryName, showInMenu);
      modified = modified || !existing || attributes(category) != previousAttributes;
    }

    const QString icon = showInMenu->GetAttribute("icon");
    pqProxyInfo* existingProxy = category->findProxy(name);
    if (existingProxy && existingProxy->isProxy() && existingProxy->group() == group &&
      (icon.isEmpty() || existingProxy->icon() == icon))
    {
      continue;
    }
    category->addProxy(new pqProxyInfo(category, name, group, "", icon, QStringList()));
    modified = true;
  }

  return modified;
}

//-----------------------------------------------------------------------------
void pqProxyCategory::warnIfScriptNameIsUsed(pqProxyCategory* category, pqProxyInfo* script)
{
  // several menu managers parse the same configurations: warn once per name.
  static QSet<QString> warnedNames;
  if (warnedNames.contains(script->name()))
  {
    return;
  }
  pqProxyCategory* root = category;
  while (root->parentCategory())
  {
    root = root->parentCategory();
  }
  for (auto item : root->getProxiesRecursive())
  {
    if (item && item->isScript() && item->name() == script->name() &&
      (item->script() != script->script() || item->fileName() != script->fileName()))
    {
      warnedNames.insert(script->name());
      qWarning() << "Script" << script->name() << "of category" << category->name()
                 << "has the same name as another script. Script names identify scripts in the "
                    "categories customized by users, so they should be unique, e.g. prefixed "
                    "by the name of the plugin.";
      return;
    }
  }
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
    for (const auto& subCategoryName : subCategories.keys())
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

  auto nameSort = [&](pqProxyCategory* categoryA, pqProxyCategory* categoryB)
  {
    QString nameA = categoryA->name().remove("&").toLower();
    QString nameB = categoryB->name().remove("&").toLower();
    return nameA < nameB;
  };

  std::sort(orderedCategories.begin(), orderedCategories.end(), nameSort);

  return orderedCategories;
}

//-----------------------------------------------------------------------------
void pqProxyCategory::diff(pqProxyCategory* base, pqProxyCategory* customized, vtkPVXMLElement* xml)
{
  auto newItemElement = [](pqProxyInfo* item)
  {
    auto element = vtkSmartPointer<vtkPVXMLElement>::New();
    element->SetName(::itemElementName(item));
    if (item->isProxy())
    {
      element->SetAttribute("group", item->group().toUtf8().data());
    }
    element->SetAttribute("name", item->name().toUtf8().data());
    return element;
  };

  // items added or modified, and removed.
  for (const auto& name : customized->OrderedProxies)
  {
    pqProxyInfo* item = customized->Proxies.value(name);
    pqProxyInfo* baseItem = base ? base->Proxies.value(name) : nullptr;
    if (!item || (baseItem && baseItem->hasSameCustomizableAttributes(item)))
    {
      continue;
    }

    auto element = newItemElement(item);
    if (!baseItem)
    {
      element->SetAttribute("added", "1");
    }
    if (!baseItem || baseItem->icon() != item->icon())
    {
      element->SetAttribute("icon", item->icon().toUtf8().data());
    }
    element->SetAttribute("hide_from_menu", item->hideFromMenu() ? "1" : "0");
    element->SetAttribute(
      "omit_from_toolbar", item->omitFromToolbar().contains(customized->Name) ? "1" : "0");
    xml->AddNestedElement(element);
  }
  if (base)
  {
    for (const auto& name : base->OrderedProxies)
    {
      pqProxyInfo* baseItem = base->Proxies.value(name);
      if (baseItem && !customized->Proxies.contains(name))
      {
        auto element = newItemElement(baseItem);
        element->SetAttribute("removed", "1");
        xml->AddNestedElement(element);
      }
    }
  }

  // categories added or modified, and removed.
  for (auto category : customized->SubCategories)
  {
    pqProxyCategory* baseCategory = base ? base->findSubCategory(category->Name) : nullptr;

    vtkNew<vtkPVXMLElement> element;
    element->SetName("Category");
    element->SetAttribute("name", category->Name.toUtf8().data());
    bool modified = false;
    if (!baseCategory)
    {
      element->SetAttribute("added", "1");
      modified = true;
    }
    if (!baseCategory || baseCategory->Label != category->Label)
    {
      element->SetAttribute("menu_label", category->Label.toUtf8().data());
      modified = true;
    }
    auto addFlag = [&](const char* attribute, bool value, bool baseValue)
    {
      if (!baseCategory || value != baseValue)
      {
        element->SetAttribute(attribute, value ? "1" : "0");
        modified = true;
      }
    };
    addFlag(
      "show_in_toolbar", category->ShowInToolbar, baseCategory && baseCategory->ShowInToolbar);
    addFlag(
      "show_in_menubar", category->ShowInMenuBar, baseCategory && baseCategory->ShowInMenuBar);
    addFlag("preserve_order", category->PreserveOrder, baseCategory && baseCategory->PreserveOrder);

    pqProxyCategory::diff(baseCategory, category, element);
    if (modified || element->GetNumberOfNestedElements() > 0)
    {
      xml->AddNestedElement(element);
    }
  }
  if (base)
  {
    for (auto baseCategory : base->SubCategories)
    {
      if (!customized->SubCategories.contains(baseCategory->Name))
      {
        vtkNew<vtkPVXMLElement> element;
        element->SetName("Category");
        element->SetAttribute("name", baseCategory->Name.toUtf8().data());
        element->SetAttribute("removed", "1");
        xml->AddNestedElement(element);
      }
    }
  }

  // keep the customizations that could not be applied, e.g. for categories and
  // items of plugins that are not loaded, unless the user customized the same
  // category or item since.
  vtkPVXMLElement* unapplied = customized->UnappliedCustomizations;
  for (unsigned int cc = 0; unapplied && cc < unapplied->GetNumberOfNestedElements(); ++cc)
  {
    vtkPVXMLElement* element = unapplied->GetNestedElement(cc);
    const QString name = element->GetAttribute("name");
    const bool customizedSince = ::isCategoryElement(element)
      ? customized->SubCategories.contains(name)
      : customized->Proxies.contains(name);
    if (!customizedSince && !::hasElementFor(xml, element))
    {
      vtkNew<vtkPVXMLElement> copy;
      element->CopyTo(copy);
      xml->AddNestedElement(copy);
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxyCategory::applyCustomizations(vtkPVXMLElement* xml, pqProxyCategory* base)
{
  for (unsigned int cc = 0; cc < xml->GetNumberOfNestedElements(); ++cc)
  {
    vtkPVXMLElement* element = xml->GetNestedElement(cc);
    const char* name = element->GetAttribute("name");
    if (!name || !element->GetName())
    {
      continue;
    }
    int added = 0;
    element->GetScalarAttribute("added", &added);
    int removed = 0;
    element->GetScalarAttribute("removed", &removed);
    int flag = 0;

    if (::isCategoryElement(element))
    {
      pqProxyCategory* category = this->findSubCategory(name);
      if (removed == 1)
      {
        if (category)
        {
          this->removeCategory(name);
          category->deleteLater();
        }
        else
        {
          // e.g. removal of a category defined by a plugin that is not loaded.
          this->keepUnappliedCustomization(element);
        }
        continue;
      }
      if (!category)
      {
        if (added != 1)
        {
          // customization of a category missing from the base e.g. defined by
          // a plugin that is not loaded.
          this->keepUnappliedCustomization(element);
          continue;
        }
        category = new pqProxyCategory(this, name, element->GetAttribute("menu_label"));
      }
      else if (element->GetAttribute("menu_label"))
      {
        category->updateLabel(element->GetAttribute("menu_label"));
      }
      if (element->GetScalarAttribute("show_in_toolbar", &flag))
      {
        category->ShowInToolbar = (flag == 1);
      }
      if (element->GetScalarAttribute("show_in_menubar", &flag))
      {
        category->ShowInMenuBar = (flag == 1);
      }
      if (element->GetScalarAttribute("preserve_order", &flag))
      {
        category->PreserveOrder = (flag == 1);
      }
      category->applyCustomizations(element, base);
    }
    else if (::isItemElement(element))
    {
      pqProxyInfo* item = this->Proxies.value(name);
      if (removed == 1)
      {
        if (item)
        {
          this->deleteProxy(name);
        }
        else
        {
          // e.g. removal of an item added by a plugin that is not loaded.
          this->keepUnappliedCustomization(element);
        }
        continue;
      }
      if (!item)
      {
        if (added != 1)
        {
          // e.g. customization of an item added by a plugin that is not loaded.
          this->keepUnappliedCustomization(element);
          continue;
        }
        item = new pqProxyInfo(this, element);
        // scripts are identified by name, their definition comes from the base.
        pqProxyInfo* definition = base ? base->findProxy(name, true) : nullptr;
        if (item->isScript() && definition && definition->isScript())
        {
          item->merge(definition);
        }
        if (!item->isValid())
        {
          // e.g. a script defined by a plugin that is not loaded.
          delete item;
          this->keepUnappliedCustomization(element);
          continue;
        }
        this->addProxy(item);
      }
      if (element->GetAttribute("icon"))
      {
        item->setIcon(element->GetAttribute("icon"));
      }
      if (element->GetScalarAttribute("hide_from_menu", &flag))
      {
        item->setHideFromMenu(flag == 1);
      }
      if (element->GetScalarAttribute("omit_from_toolbar", &flag))
      {
        item->setOmitFromToolbar(this->Name, flag == 1);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxyCategory::keepUnappliedCustomization(vtkPVXMLElement* element)
{
  if (!this->UnappliedCustomizations)
  {
    this->UnappliedCustomizations = vtkSmartPointer<vtkPVXMLElement>::New();
    this->UnappliedCustomizations->SetName("UnappliedCustomizations");
  }
  vtkNew<vtkPVXMLElement> copy;
  element->CopyTo(copy);
  this->UnappliedCustomizations->AddNestedElement(copy);
}

//-----------------------------------------------------------------------------
void pqProxyCategory::deleteProxy(const QString& name)
{
  pqProxyInfo* item = this->Proxies.value(name);
  this->removeProxy(name);
  if (item)
  {
    // the item is already removed, and another one may be added with the same name.
    QObject::disconnect(item, nullptr, this, nullptr);
    item->deleteLater();
  }
}

//-----------------------------------------------------------------------------
void pqProxyCategory::writeSettings(const QString& resourceTag, pqProxyCategory* base)
{
  vtkNew<vtkPVXMLElement> root;
  root->SetName("CategoryCustomizations");
  pqProxyCategory::diff(base, this, root);

  std::stringstream sstream;
  root->PrintXML(sstream, vtkIndent());

  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue(
    ::SETTINGS_CUSTOMIZATIONS_KEY.arg(resourceTag), QString::fromStdString(sstream.str()));
  vtkLogF(TRACE, "Category customizations written to settings.");
  settings->alertSettingsModified();
}

//-----------------------------------------------------------------------------
void pqProxyCategory::loadSettings(const QString& resourceTag, pqProxyCategory* base)
{
  this->clear();
  if (base)
  {
    this->deepCopy(base);
  }

  if (auto customizations = ::parseSettings(::SETTINGS_CUSTOMIZATIONS_KEY.arg(resourceTag)))
  {
    this->applyCustomizations(customizations, base);
    vtkLogF(TRACE, "Category customizations loaded from settings.");
  }
  else if (auto legacy = ::parseSettings(::SETTINGS_CATEGORY_KEY.arg(resourceTag)))
  {
    // Previous versions saved a copy of the whole categories: merge it in.
    this->parseXML(legacy);
    vtkLogF(TRACE, "Categories loaded from legacy settings.");
  }
}

//-----------------------------------------------------------------------------
QString pqProxyCategory::savedCustomizations(const QString& resourceTag)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  return settings->value(::SETTINGS_CUSTOMIZATIONS_KEY.arg(resourceTag)).toString() + '\n' +
    settings->value(::SETTINGS_CATEGORY_KEY.arg(resourceTag)).toString();
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

//-----------------------------------------------------------------------------
void pqProxyCategory::setShowInMenuBar(bool show)
{
  this->ShowInMenuBar = show;
}

//-----------------------------------------------------------------------------
int pqProxyCategory::separatorCount()
{
  int count = 0;
  for (auto proxy : this->Proxies)
  {
    if (proxy && proxy->isSeparator())
    {
      ++count;
    }
  }
  return count;
}
