/*=========================================================================

   Program: ParaView
   Module:    pqProxyGroupMenuManager.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqProxyGroupMenuManager.h"

#include "pqActiveObjects.h"
#include "pqAddToFavoritesReaction.h"
#include "pqCoreUtilities.h"
#include "pqManageFavoritesReaction.h"
#include "pqPVApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqSetData.h"
#include "pqSetName.h"
#include "pqSettings.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#include "vtkNew.h"
#include "vtkSmartPointer.h"

#include <QApplication>
#include <QMap>
#include <QMenu>
#include <QPair>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QtDebug>

class pqProxyGroupMenuManager::pqInternal
{
public:
  struct Info
  {
    QString Icon; //<-- Name of the icon to use, if any.
    QStringList
      OmitFromToolbar; //<-- a list of category names whose toolbars should not contain this action.
    QPointer<QAction> Action; //<-- Action for this proxy.
  };

  typedef QMap<QPair<QString, QString>, Info> ProxyInfoMap;

  struct CategoryInfo
  {
    QString Label;
    bool PreserveOrder;
    bool ShowInToolbar;
    bool HideForTests;
    QList<QPair<QString, QString> > Proxies;
    CategoryInfo()
    {
      this->PreserveOrder = false;
      this->ShowInToolbar = false;
      this->HideForTests = false;
    }
  };

  typedef QMap<QString, CategoryInfo> CategoryInfoMap;

  pqInternal() { this->LocalActiveSession = NULL; }

  void addProxy(const QString& pgroup, const QString& pname, const QString& icon,
    const QString& omitFromToolbar = QString())
  {
    if (!pname.isEmpty() && !pgroup.isEmpty())
    {
      Info& info = this->Proxies[QPair<QString, QString>(pgroup, pname)];
      if (!omitFromToolbar.isEmpty())
      {
        info.OmitFromToolbar << omitFromToolbar;
      }
      if (!icon.isEmpty())
      {
        info.Icon = icon;
      }
    }
  }

  void removeProxy(const QString& pgroup, const QString& pname)
  {
    if (!pname.isEmpty() && !pgroup.isEmpty())
    {
      QPair<QString, QString> pair(pgroup, pname);
      this->Proxies.remove(pair);
    }
  }

  // Proxies and Categories is what gets shown in the menu.
  ProxyInfoMap Proxies;
  CategoryInfoMap Categories;
  QList<QPair<QString, QString> > RecentlyUsed;
  // list of favorites. Each pair is {filterGroup, filterPath} where filterPath
  // is the category path to access the favorite: category1;category2;...;filterName
  QList<QPair<QString, QString> > Favorites;
  QSet<QString> ProxyDefinitionGroupToListen;
  QSet<unsigned long> CallBackIDs;
  QWidget Widget;
  QPointer<QAction> SearchAction;
  unsigned long ProxyManagerCallBackId;
  void* LocalActiveSession;

  QPointer<QMenu> RecentMenu;
  QPointer<QMenu> FavoritesMenu;
};

//-----------------------------------------------------------------------------
pqProxyGroupMenuManager::pqProxyGroupMenuManager(
  QMenu* _menu, const QString& resourceTagName, bool quickLaunchable)
  : Superclass(_menu)
  , SupportsQuickLaunch(quickLaunchable)
{
  this->ResourceTagName = resourceTagName;
  this->Internal = new pqInternal();
  this->RecentlyUsedMenuSize = 0;
  this->Enabled = true;
  this->EnableFavorites = false;

  QObject::connect(pqApplicationCore::instance(), SIGNAL(loadXML(vtkPVXMLElement*)), this,
    SLOT(loadConfiguration(vtkPVXMLElement*)));

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverRemoved(pqServer*)), this, SLOT(removeProxyDefinitionUpdateObservers()));

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(addProxyDefinitionUpdateObservers()));

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), this,
    SLOT(lookForNewDefinitions()));

  this->Internal->ProxyManagerCallBackId =
    pqCoreUtilities::connect(vtkSMProxyManager::GetProxyManager(),
      vtkSMProxyManager::ActiveSessionChanged, this, SLOT(switchActiveServer()));

  QObject::connect(this->menu(), SIGNAL(aboutToShow()), this, SLOT(updateMenuStyle()));

  // register with pqPVApplicationCore for quicklaunch, if enabled.
  auto* pvappcore = pqPVApplicationCore::instance();
  if (quickLaunchable && pvappcore)
  {
    pvappcore->registerForQuicklaunch(this->widgetActionsHolder());
  }
}

//-----------------------------------------------------------------------------
pqProxyGroupMenuManager::~pqProxyGroupMenuManager()
{
  this->removeProxyDefinitionUpdateObservers();
  if (vtkSMProxyManager::IsInitialized())
  {
    vtkSMProxyManager::GetProxyManager()->RemoveObserver(this->Internal->ProxyManagerCallBackId);
  }
  delete this->Internal;
  this->Internal = 0;
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::addProxy(const QString& xmlgroup, const QString& xmlname)
{
  this->Internal->addProxy(xmlgroup.toLocal8Bit().data(), xmlname.toLocal8Bit().data(), QString());
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::removeProxy(const QString& xmlgroup, const QString& xmlname)
{
  this->Internal->removeProxy(xmlgroup.toLocal8Bit().data(), xmlname.toLocal8Bit().data());
}

//-----------------------------------------------------------------------------
namespace
{
void pqProxyGroupMenuManagerConvertLegacyXML(vtkPVXMLElement* root)
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
    pqProxyGroupMenuManagerConvertLegacyXML(root->GetNestedElement(cc));
  }
}
};

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::loadConfiguration(vtkPVXMLElement* root)
{
  if (!root || !root->GetName())
  {
    return;
  }
  if (this->ResourceTagName != root->GetName())
  {
    this->loadConfiguration(
      root->FindNestedElementByName(this->ResourceTagName.toLocal8Bit().data()));
    return;
  }

  // Convert legacy xml to new style.
  pqProxyGroupMenuManagerConvertLegacyXML(root);

  // Iterate over Category elements and find items with tag name "Proxy".
  // Iterate over elements with tag "Proxy" and add them to the
  // this->Internal->Proxies map.
  unsigned int numElems = root->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    vtkPVXMLElement* curElem = root->GetNestedElement(cc);
    if (!curElem || !curElem->GetName())
    {
      continue;
    }

    if (strcmp(curElem->GetName(), "Category") == 0 && curElem->GetAttribute("name"))
    {
      // We need to be certain if this group is for the elements we are concerned
      // with. i.e. is there at least one element with tag "Proxy" in this
      // category?
      if (!curElem->FindNestedElementByName("Proxy"))
      {
        continue;
      }
      QString categoryName = curElem->GetAttribute("name");
      QString categoryLabel =
        curElem->GetAttribute("menu_label") ? curElem->GetAttribute("menu_label") : categoryName;
      int preserve_order = 0;
      curElem->GetScalarAttribute("preserve_order", &preserve_order);
      int show_in_toolbar = 0;
      curElem->GetScalarAttribute("show_in_toolbar", &show_in_toolbar);
      int hide_for_tests = 0;
      curElem->GetScalarAttribute("hide_for_tests", &hide_for_tests);

      // Valid category encountered. Update the Internal datastructures.
      pqInternal::CategoryInfo& category = this->Internal->Categories[categoryName];
      category.Label = categoryLabel;
      category.PreserveOrder = category.PreserveOrder || (preserve_order == 1);
      category.ShowInToolbar = category.ShowInToolbar || (show_in_toolbar == 1);
      category.HideForTests = category.HideForTests || (hide_for_tests == 1);
      unsigned int numCategoryElems = curElem->GetNumberOfNestedElements();
      for (unsigned int kk = 0; kk < numCategoryElems; ++kk)
      {
        vtkPVXMLElement* child = curElem->GetNestedElement(kk);
        if (child && child->GetName() && strcmp(child->GetName(), "Proxy") == 0)
        {
          const char* name = child->GetAttribute("name");
          const char* group = child->GetAttribute("group");
          const char* icon = child->GetAttribute("icon");
          int omit = 0;
          child->GetScalarAttribute("omit_from_toolbar", &omit);
          if (!name || !group)
          {
            continue;
          }
          this->Internal->addProxy(group, name, icon, omit ? categoryName : QString());
          if (!category.Proxies.contains(QPair<QString, QString>(group, name)))
          {
            category.Proxies.push_back(QPair<QString, QString>(group, name));
          }
        }
      }
    }
    else if (strcmp(curElem->GetName(), "Proxy") == 0)
    {
      const char* name = curElem->GetAttribute("name");
      const char* group = curElem->GetAttribute("group");
      const char* icon = curElem->GetAttribute("icon");
      if (!name || !group)
      {
        continue;
      }
      this->Internal->addProxy(group, name, icon);
    }
  }

  this->populateMenu();
}

static bool actionTextSort(QAction* a, QAction* b)
{
  return a->text() < b->text();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::populateRecentlyUsedMenu()
{
  // doing this here, ensure that even if multiple pqProxyGroupMenuManager
  // instances exists for same `resourceTagName`, the recent list remains synced
  // between all.
  this->loadRecentlyUsedItems();
  if (QMenu* recentMenu = this->Internal->RecentMenu)
  {
    recentMenu->clear();
    for (const QPair<QString, QString>& key : this->Internal->RecentlyUsed)
    {
      if (auto action = this->getAction(key.first, key.second))
      {
        recentMenu->addAction(action);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::loadRecentlyUsedItems()
{
  this->Internal->RecentlyUsed.clear();
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = QString("recent.%1/").arg(this->ResourceTagName);
  if (settings->contains(key))
  {
    QString list = settings->value(key).toString();
    QStringList parts = list.split("|", QString::SkipEmptyParts);
    foreach (QString part, parts)
    {
      QStringList pieces = part.split(";", QString::SkipEmptyParts);
      if (pieces.size() == 2)
      {
        QPair<QString, QString> aKey(pieces[0], pieces[1]);
        this->Internal->RecentlyUsed.push_back(aKey);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::saveRecentlyUsedItems()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = QString("recent.%1/").arg(this->ResourceTagName);
  QString value;
  for (int cc = 0; cc < this->Internal->RecentlyUsed.size(); cc++)
  {
    value += QString("%1;%2|")
               .arg(this->Internal->RecentlyUsed[cc].first)
               .arg(this->Internal->RecentlyUsed[cc].second);
  }
  settings->setValue(key, value);
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::populateFavoritesMenu()
{
  this->loadFavoritesItems();
  if (this->Internal->FavoritesMenu)
  {
    this->Internal->FavoritesMenu->clear();

    QAction* manageFavoritesAction =
      this->Internal->FavoritesMenu->addAction("&Manage Favorites...")
      << pqSetName("actionManage_Favorites");
    new pqManageFavoritesReaction(manageFavoritesAction, this);

    this->Internal->FavoritesMenu->addAction(this->getAddToCategoryAction(QString()));
    this->Internal->FavoritesMenu->addSeparator();

    for (const QPair<QString, QString>& key : this->Internal->Favorites)
    {
      QStringList categories = key.second.split(";", QString::SkipEmptyParts);
      bool isCategory = key.first.compare("categories") == 0;
      QString filter = isCategory ? QString("") : categories.takeLast();
      if (!isCategory)
      {
        categories.removeLast();
      }

      QMenu* submenu = this->Internal->FavoritesMenu;
      for (const QString& category : categories)
      {
        bool submenuExists = false;
        for (QAction* submenuAction : submenu->actions())
        {
          if (submenuAction->menu() && submenuAction->menu()->objectName() == category)
          {
            // if category menu already exists, use it
            submenu = submenuAction->menu();
            submenuExists = true;
            break;
          }
        }
        if (!submenuExists)
        {
          submenu = submenu->addMenu(category) << pqSetName(category);
          QString path = categories.join(";");
          submenu->addAction(this->getAddToCategoryAction(path));
          submenu->addSeparator();
        }
      }

      // if favorite does not exist (e.g. filter from an unloaded plugin)
      // no action will be created. (but favorite stays in memory)
      auto action = isCategory ? nullptr : this->getAction(key.first, filter);
      if (action)
      {
        action->setObjectName(filter);
        submenu->addAction(action);
      }
    }
  }
}

//-----------------------------------------------------------------------------
QAction* pqProxyGroupMenuManager::getAddToCategoryAction(const QString& path)
{
  QAction* actionAddToFavorites = new QAction(this);
  actionAddToFavorites->setObjectName(QString("actionAddTo:%1").arg(path));
  actionAddToFavorites->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "&Add current filter", Q_NULLPTR));
  actionAddToFavorites->setData(path);

  // get filters list for current category
  QVector<QString> filters;
  for (const QPair<QString, QString>& key : this->Internal->Favorites)
  {
    if (key.first == "filters")
    {
      QStringList categories = key.second.split(";", QString::SkipEmptyParts);
      QString filter = categories.takeLast();
      categories.removeLast();
      if (path == categories.join(";"))
      {
        filters << filter;
      }
    }
  }

  new pqAddToFavoritesReaction(actionAddToFavorites, filters);

  return actionAddToFavorites;
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::loadFavoritesItems()
{
  this->Internal->Favorites.clear();
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = QString("favorites.%1/").arg(this->ResourceTagName);
  if (settings->contains(key))
  {
    QString list = settings->value(key).toString();
    QStringList parts = list.split("|", QString::SkipEmptyParts);
    for (const QString& part : parts)
    {
      QStringList pieces = part.split(";", QString::SkipEmptyParts);
      if (pieces.size() >= 2)
      {
        QString group = pieces.takeFirst();
        QString path = pieces.join(";");
        QPair<QString, QString> aKey(group, path);
        this->Internal->Favorites.push_back(aKey);
      }
    }
  }

  this->updateMenuStyle();
}

//-----------------------------------------------------------------------------
QMenu* pqProxyGroupMenuManager::getFavoritesMenu()
{
  return this->Internal->FavoritesMenu;
}

//-----------------------------------------------------------------------------
QString pqProxyGroupMenuManager::categoryLabel(const QString& category)
{
  if (this->Internal->Categories.contains(category))
  {
    return this->Internal->Categories[category].Label;
  }

  return QString();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::populateMenu()
{
  // We reuse QAction instances, yet we don't want to have callbacks set up for
  // actions that are no longer shown in the menu. Hence we disconnect all
  // signal connections.
  QMenu* _menu = this->menu();

  QList<QAction*> menuActions = _menu->actions();
  foreach (QAction* action, menuActions)
  {
    QObject::disconnect(action, 0, this, 0);
  }
  menuActions.clear();
  if (!this->Internal->SearchAction.isNull())
  {
    this->Internal->SearchAction->deleteLater();
  }

  QList<QMenu*> submenus = _menu->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
  foreach (QMenu* submenu, submenus)
  {
    delete submenu;
  }
  _menu->clear();

  if (this->supportsQuickLaunch())
  {
#if defined(Q_WS_MAC) || defined(Q_OS_MAC)
    this->Internal->SearchAction =
      _menu->addAction("Search...\tAlt+Space", this, SLOT(quickLaunch()));
#else
    this->Internal->SearchAction =
      _menu->addAction("Search...\tCtrl+Space", this, SLOT(quickLaunch()));
#endif
  }

  if (this->RecentlyUsedMenuSize > 0)
  {
    auto* rmenu = _menu->addMenu("&Recent") << pqSetName("Recent");
    this->Internal->RecentMenu = rmenu;
    this->connect(rmenu, SIGNAL(aboutToShow()), SLOT(populateRecentlyUsedMenu()));
  }

  if (this->EnableFavorites)
  {
    auto* bmenu = _menu->addMenu("&Favorites") << pqSetName("Favorites");
    this->Internal->FavoritesMenu = bmenu;
    this->connect(_menu, SIGNAL(aboutToShow()), SLOT(populateFavoritesMenu()));
  }

  _menu->addSeparator();

  // Add alphabetical list.
  QMenu* alphabeticalMenu = _menu;
  if (this->Internal->Categories.size() > 0 || this->RecentlyUsedMenuSize > 0)
  {
    alphabeticalMenu = _menu->addMenu("&Alphabetical") << pqSetName("Alphabetical");
  }

  pqInternal::ProxyInfoMap::iterator proxyIter = this->Internal->Proxies.begin();

  QList<QAction*> someActions;
  for (; proxyIter != this->Internal->Proxies.end(); ++proxyIter)
  {
    QAction* action = this->getAction(proxyIter.key().first, proxyIter.key().second);
    if (action)
    {
      someActions.push_back(action);
    }
  }

  // Now sort all actions added in temp based on their texts.
  qSort(someActions.begin(), someActions.end(), ::actionTextSort);
  foreach (QAction* action, someActions)
  {
    alphabeticalMenu->addAction(action);
  }

  // Add categories.
  pqInternal::CategoryInfoMap::iterator categoryIter = this->Internal->Categories.begin();
  for (; categoryIter != this->Internal->Categories.end(); ++categoryIter)
  {
    QList<QAction*> action_list = this->actions(categoryIter.key());
    if (action_list.size() > 0)
    {
      QMenu* categoryMenu = _menu->addMenu(categoryIter.value().Label)
        << pqSetName(categoryIter.key());
      foreach (QAction* action, action_list)
      {
        categoryMenu->addAction(action);
      }
    }
  }

  emit this->menuPopulated();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::updateMenuStyle()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  bool sc = settings->value("GeneralSettings.ForceSingleColumnMenus", false).toBool();
  this->menu()->setStyleSheet(QString("QMenu { menu-scrollable: %1; }").arg(sc ? 1 : 0));

  for (QAction* action : this->actions())
  {
    QFont f = action->font();
    f.setBold(false);
    action->setFont(f);
  }

  for (auto bm : this->Internal->Favorites)
  {
    QStringList path = bm.second.split(";", QString::SkipEmptyParts);
    QString filter = path.takeLast();
    if (QAction* action = this->getAction(bm.first, filter))
    {
      QFont f = action->font();
      f.setBold(true);
      action->setFont(f);
    }
  }
}

//-----------------------------------------------------------------------------
QAction* pqProxyGroupMenuManager::getAction(const QString& pgroup, const QString& pname)
{
  if (pname.isEmpty() || pgroup.isEmpty())
  {
    return 0;
  }

  // Since Proxies map keeps the QAction instance, we will reuse the QAction
  // instance whenever possible.
  QPair<QString, QString> key(pgroup, pname);
  pqInternal::ProxyInfoMap::iterator iter = this->Internal->Proxies.find(key);
  QString name = QString("%1").arg(pname);
  if (iter == this->Internal->Proxies.end())
  {
    return 0;
  }

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if (!pxm)
  {
    return 0;
  }
  vtkSMProxy* prototype =
    pxm->GetPrototypeProxy(pgroup.toLocal8Bit().data(), pname.toLocal8Bit().data());
  if (prototype)
  {
    QString label = prototype->GetXMLLabel() ? prototype->GetXMLLabel() : pname;
    QAction* action = iter.value().Action;
    if (!action)
    {
      action = new QAction(this);
      QStringList data_list;
      data_list << pgroup << pname;
      action << pqSetName(name) << pqSetData(data_list);
      pqSettings settings;
      if (pgroup == "filters" || pgroup == "sources")
      {
        QString menuName = pgroup == "filters" ? "Filters" : "Sources";
        auto variant = settings.value(
          QString("pqCustomShortcuts/%1/Alphabetical/%2").arg(menuName, label), QVariant());
        if (variant.canConvert<QKeySequence>())
        {
          action->setShortcut(variant.value<QKeySequence>());
        }
      }
      if (iter.value().OmitFromToolbar.size() > 0)
      {
        action->setProperty("OmitFromToolbar", iter.value().OmitFromToolbar);
      }
      iter.value().Action = action;
    }

    // Add action in the pool for the QuickSearch...
    this->Internal->Widget.addAction(action);

    action->setText(label);
    QString icon = this->Internal->Proxies[key].Icon;

    // Try to add some default icons if none is specified.
    if (icon.isEmpty() && prototype->IsA("vtkSMCompoundSourceProxy"))
    {
      icon = ":/pqWidgets/Icons/pqBundle32.png";
    }

    if (!icon.isEmpty())
    {
      action->setIcon(QIcon(icon));
    }

    // this avoids creating duplicate connections.
    this->connect(action, SIGNAL(triggered()), SLOT(triggered()), Qt::UniqueConnection);
    return action;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::triggered()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if (!action)
  {
    return;
  }
  QStringList data_list = action->data().toStringList();
  if (data_list.size() != 2)
  {
    return;
  }
  QPair<QString, QString> key(data_list[0], data_list[1]);
  emit this->triggered(key.first, key.second);
  if (this->RecentlyUsedMenuSize > 0)
  {
    this->Internal->RecentlyUsed.removeAll(key);
    this->Internal->RecentlyUsed.push_front(key);
    while (this->Internal->RecentlyUsed.size() > static_cast<int>(this->RecentlyUsedMenuSize))
    {
      this->Internal->RecentlyUsed.pop_back();
    }
    this->saveRecentlyUsedItems();

    // while this is not necessary, this overcomes a limitation of our testing
    // framework where it doesn't trigger "aboutToShow" signal.
    this->populateRecentlyUsedMenu();
  }
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::quickLaunch()
{
  if (this->supportsQuickLaunch() && pqPVApplicationCore::instance())
  {
    pqPVApplicationCore::instance()->quickLaunch();
  }
}

//-----------------------------------------------------------------------------
QWidget* pqProxyGroupMenuManager::widgetActionsHolder() const
{
  return &this->Internal->Widget;
}

//-----------------------------------------------------------------------------
QList<QAction*> pqProxyGroupMenuManager::actions() const
{
  return this->widgetActionsHolder()->actions();
}

//-----------------------------------------------------------------------------
bool pqProxyGroupMenuManager::hideForTests(const QString& category) const
{
  pqInternal::CategoryInfoMap::iterator categoryIter = this->Internal->Categories.find(category);
  if (categoryIter == this->Internal->Categories.end() ||
    (categoryIter.value().ShowInToolbar && !categoryIter.value().HideForTests))
  {
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqProxyGroupMenuManager::getPrototype(QAction* action) const
{
  if (!action)
  {
    return NULL;
  }
  QStringList data_list = action->data().toStringList();
  if (data_list.size() != 2)
  {
    return NULL;
  }

  QPair<QString, QString> key(data_list[0], data_list[1]);
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  return pxm->GetPrototypeProxy(key.first.toLocal8Bit().data(), key.second.toLocal8Bit().data());
}

//-----------------------------------------------------------------------------
QStringList pqProxyGroupMenuManager::getToolbarCategories() const
{
  QStringList categories_in_toolbar;

  pqInternal::CategoryInfoMap::iterator categoryIter = this->Internal->Categories.begin();
  for (; categoryIter != this->Internal->Categories.end(); ++categoryIter)
  {
    if (categoryIter.value().ShowInToolbar)
    {
      categories_in_toolbar.push_back(categoryIter.key());
    }
  }
  return categories_in_toolbar;
}

//-----------------------------------------------------------------------------
QList<QAction*> pqProxyGroupMenuManager::actions(const QString& category)
{
  QList<QAction*> category_actions;
  pqInternal::CategoryInfoMap::iterator categoryIter = this->Internal->Categories.find(category);
  if (categoryIter == this->Internal->Categories.end())
  {
    return category_actions;
  }

  for (int cc = 0; cc < categoryIter.value().Proxies.size(); cc++)
  {
    QPair<QString, QString> pname = categoryIter.value().Proxies[cc];
    QAction* action = this->getAction(pname.first, pname.second);
    if (action)
    {
      // build an action list, so that we can sort it and then add to the
      // menu (BUG #8364).
      category_actions.push_back(action);
    }
  }
  if (categoryIter.value().PreserveOrder == false)
  {
    // sort unless the XML overrode the sorting using the "preserve_order"
    // attribute.
    qSort(category_actions.begin(), category_actions.end(), ::actionTextSort);
  }
  return category_actions;
}

QList<QAction*> pqProxyGroupMenuManager::actionsInToolbars()
{
  QList<QAction*> actions_in_toolbars;
  for (pqInternal::CategoryInfoMap::iterator categoryIter = this->Internal->Categories.begin();
       categoryIter != this->Internal->Categories.end(); ++categoryIter)
  {
    const QString& categoryName = categoryIter.key();
    pqInternal::CategoryInfo& category = categoryIter.value();
    if (category.ShowInToolbar)
    {
      QPair<QString, QString> pname;
      foreach (pname, category.Proxies)
      {
        QAction* action = this->getAction(pname.first, pname.second);
        if (action)
        {
          QVariant v = action->property("OmitFromToolbar");
          if (!v.isValid() || !v.toStringList().contains(categoryName))
          {
            if (!actions_in_toolbars.contains(action))
            {
              actions_in_toolbars.push_back(action);
            }
          }
        }
      }
    }
  }

  return actions_in_toolbars;
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::setEnabled(bool enable)
{
  this->Enabled = enable;
  // on Mac, with Qt 4.8.1, the enabling/disabling of the menu itself causes
  // issues; the menu never re-enables itself after being disabled (BUG #13184).

  // Furthermore, with the change to recomputing the enabled state when the menu
  // is shown using the aboutToShow signal, disabling the menu itself causes the
  // signal not to be sent resulting in the menu never being re-enabled.
  //  this->menu()->setEnabled(enable);
}
//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::addProxyDefinitionUpdateListener(const QString& proxyGroupName)
{
  this->Internal->ProxyDefinitionGroupToListen.insert(proxyGroupName);
  this->removeProxyDefinitionUpdateObservers();
  this->addProxyDefinitionUpdateObservers();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::removeProxyDefinitionUpdateListener(const QString& proxyGroupName)
{
  this->Internal->ProxyDefinitionGroupToListen.remove(proxyGroupName);
  this->removeProxyDefinitionUpdateObservers();
  this->addProxyDefinitionUpdateObservers();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::removeProxyDefinitionUpdateObservers()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  foreach (unsigned long callbackID, this->Internal->CallBackIDs)
  {
    pxm->RemoveObserver(callbackID);
  }
  this->Internal->CallBackIDs.clear();
}
//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::addProxyDefinitionUpdateObservers()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Regular proxy
  unsigned long callbackID = pxm->AddObserver(vtkSMProxyDefinitionManager::ProxyDefinitionsUpdated,
    this, &pqProxyGroupMenuManager::lookForNewDefinitions);
  this->Internal->CallBackIDs.insert(callbackID);

  // compound proxy
  callbackID = pxm->AddObserver(vtkSMProxyDefinitionManager::CompoundProxyDefinitionsUpdated, this,
    &pqProxyGroupMenuManager::lookForNewDefinitions);
  this->Internal->CallBackIDs.insert(callbackID);

  // Look inside the definition
  this->lookForNewDefinitions();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::lookForNewDefinitions()
{
  // Look inside the group name that are tracked
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  if (this->Internal->ProxyDefinitionGroupToListen.size() == 0 || pxm == NULL)
  {
    return; // Nothing to look into...
  }
  vtkSMProxyDefinitionManager* pxdm = pxm->GetProxyDefinitionManager();

  // Setup definition iterator
  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pxdm->NewIterator());
  foreach (QString groupName, this->Internal->ProxyDefinitionGroupToListen)
  {
    iter->AddTraversalGroupName(groupName.toLocal8Bit().data());
  }

  // Loop over proxy that should be inserted inside the UI
  QSet<QPair<QString, QString> > definitionSet;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    const char* group = iter->GetGroupName();
    const char* name = iter->GetProxyName();
    vtkPVXMLElement* hints = iter->GetProxyHints();
    if (hints != NULL)
    {
      if (hints->FindNestedElementByName("ReaderFactory") != NULL)
      {
        // skip readers.
        continue;
      }
      for (unsigned int cc = 0; cc < hints->GetNumberOfNestedElements(); cc++)
      {
        vtkPVXMLElement* showInMenu = hints->GetNestedElement(cc);
        if (showInMenu == NULL || showInMenu->GetName() == NULL ||
          strcmp(showInMenu->GetName(), "ShowInMenu") != 0)
        {
          continue;
        }

        definitionSet.insert(QPair<QString, QString>(group, name));
        this->Internal->addProxy(group, name, showInMenu->GetAttribute("icon"));
        if (const char* categoryName = showInMenu->GetAttribute("category"))
        {
          pqInternal::CategoryInfo& category = this->Internal->Categories[categoryName];
          // If no label just make it up
          if (category.Label.isEmpty())
          {
            category.Label = categoryName;
          }
          int show_in_toolbar = 0;
          if (showInMenu->GetScalarAttribute("show_in_toolbar", &show_in_toolbar))
          {
            category.ShowInToolbar = category.ShowInToolbar || (show_in_toolbar == 1);
          }
          if (!category.Proxies.contains(QPair<QString, QString>(group, name)))
          {
            category.Proxies.push_back(QPair<QString, QString>(group, name));
          }
        }
      }
    }
  }
  // Update the menu with the current definition
  this->populateMenu();
}
//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::switchActiveServer()
{
  void* newActiveSession = vtkSMProxyManager::IsInitialized()
    ? vtkSMProxyManager::GetProxyManager()->GetActiveSession()
    : NULL;

  if (newActiveSession && newActiveSession != this->Internal->LocalActiveSession)
  {
    // Make sure we don't clear the menu twice for the same server
    this->Internal->LocalActiveSession = newActiveSession;

    // Clear the QuickSearch QAction pool...
    QList<QAction*> action_list = this->Internal->Widget.actions();
    foreach (QAction* action, action_list)
    {
      this->Internal->Widget.removeAction(action);
    }

    // Fill is back by updating the menu
    this->lookForNewDefinitions();
  }
}
