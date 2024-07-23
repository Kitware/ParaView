// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// Hide PARAVIEW_DEPRECATED_IN_5_13_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

#include "pqProxyGroupMenuManager.h"

#include "pqActiveObjects.h"
#include "pqAddToFavoritesReaction.h"
#include "pqCoreUtilities.h"
#include "pqManageFavoritesReaction.h"
#include "pqPVApplicationCore.h"
#include "pqProxyAction.h"
#include "pqProxyCategory.h"
#include "pqQtDeprecated.h"
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
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"

#include <QApplication>
#include <QMap>
#include <QMenu>
#include <QPair>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QtDebug>

#include <algorithm>

struct pqProxyGroupMenuManager::pqInternal
{

  static const char* FAVORITES_CATEGORY() { return "Favorites"; }

  pqInternal()
    : ApplicationCategory(new pqProxyCategory())
    , SettingsCategory(new pqProxyCategory())
  {
  }

  //-----------------------------------------------------------------------------
  pqProxyCategory* menuCategory()
  {
    if (this->SettingsCategory->isEmpty())
    {
      return this->ApplicationCategory.get();
    }

    return this->SettingsCategory.get();
  }

  /**
   * Return true if new proxies/categories definition should be added to the settings tree.
   * If settings are present we do not want to add application-defined proxies (loaded at startup).
   * Once client is set up, new proxies comes from plugins and should be added.
   */
  bool allowSettingsUpdate()
  {
    return this->ClientEnvironmentDone && !this->SettingsCategory->isEmpty();
  }

  // PARAVIEW_DEPRECATED_IN_5_13_0()
  // This is a copy of the deprecated member method.
  // As this is still used, but only in deprecated methods, we kept this here to avoid deprecation
  // warnings.
  void deprecatedLoadFavoritesItems(const QString& resourceTagName)
  {
    this->DeprecatedFavorites.clear();
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QString key = QString("favorites.%1/").arg(resourceTagName);
    if (settings->contains(key))
    {
      QString list = settings->value(key).toString();
      QStringList parts = list.split("|", PV_QT_SKIP_EMPTY_PARTS);
      for (const QString& part : parts)
      {
        QStringList pieces = part.split(";", PV_QT_SKIP_EMPTY_PARTS);
        if (pieces.size() >= 2)
        {
          QString group = pieces.takeFirst();
          QString path = pieces.join(";");
          QPair<QString, QString> aKey(group, path);
          this->DeprecatedFavorites.push_back(aKey);
        }
      }
    }
  }

  // PARAVIEW_DEPRECATED_IN_5_13_0()
  // This is a copy of the deprecated member method.
  // As this is still used, but only in deprecated methods, we kept this here to avoid deprecation
  // warnings.
  QAction* deprecatedGetAddToFavoritesAction(const QString& path, pqProxyGroupMenuManager* self)
  {
    QAction* actionAddToFavorites = new QAction(self);
    actionAddToFavorites->setObjectName(QString("actionAddTo:%1").arg(path));
    actionAddToFavorites->setText(QCoreApplication::translate(
      "pqPipelineBrowserContextMenu", "&Add current filter", Q_NULLPTR));
    actionAddToFavorites->setData(path);

    // get filters list for current category
    QVector<QString> filters;
    for (const QPair<QString, QString>& key : this->DeprecatedFavorites)
    {
      if (key.first == "filters")
      {
        QStringList categories = key.second.split(";", PV_QT_SKIP_EMPTY_PARTS);
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

  /**
   * Proxy action update.
   */
  ///@{
  /**
   * Update action properties from proxy information.
   * See updateActionIcon, updateActionOmitFromToolbar, updateActionShortcut
   */
  void updateAction(QAction* action, pqProxyInfo* proxyInfo)
  {
    this->updateActionShortcut(action, proxyInfo);
    this->updateActionIcon(action, proxyInfo);
    this->updateActionOmitFromToolbar(action, proxyInfo);
  }

  /**
   * Update action icon from proxy info.
   * For CustomFilters, fallback to a default icon.
   */
  void updateActionIcon(QAction* action, pqProxyInfo* proxyInfo)
  {
    const QString group = proxyInfo->group();
    const QString name = proxyInfo->name();
    QString icon = proxyInfo->icon();

    if (icon.isEmpty())
    {
      vtkSMProxy* prototype = pqProxyAction::GetProxyPrototype(action);
      // Try to add some default icons if none is specified.
      if (prototype && prototype->IsA("vtkSMCompoundSourceProxy"))
      {
        icon = ":/pqWidgets/Icons/pqBundle32.png";
      }
    }

    if (!icon.isEmpty())
    {
      action->setIcon(QIcon(icon));
    }
  }

  /**
   * Update action "omit from toolbar" property from proxy info.
   */
  void updateActionOmitFromToolbar(QAction* action, pqProxyInfo* proxy)
  {
    QStringList omittedToolbars = proxy->omitFromToolbar();
    QVariant omitFromToolbar = action->property("OmitFromToolbar");
    if (omitFromToolbar.isValid() && !omitFromToolbar.toStringList().empty())
    {
      omittedToolbars << omitFromToolbar.toStringList();
    }

    if (!omittedToolbars.empty())
    {
      action->setProperty("OmitFromToolbar", omittedToolbars);
    }
  }

  /**
   * Update action shortcut from settings.
   */
  void updateActionShortcut(QAction* action, pqProxyInfo* proxyInfo)
  {
    const QString group = proxyInfo->group();
    pqSettings* settings = pqApplicationCore::instance()->settings();

    if (group == "filters" || group == "sources")
    {
      QString menuName = group == "filters" ? "Filters" : "Sources";
      auto variant = settings->value(
        QString("pqCustomShortcuts/%1/Alphabetical/%2").arg(menuName, proxyInfo->label()),
        QVariant());
      if (variant.canConvert<QKeySequence>())
      {
        action->setShortcut(variant.value<QKeySequence>());
        action->setAutoRepeat(false);
      }
    }
  }
  ///@}

  bool hasFavorites(pqProxyCategory* parentCategory)
  {
    return parentCategory->findSubCategory(pqInternal::FAVORITES_CATEGORY()) != nullptr;
  }

  void createFavorites(pqProxyCategory* parentCategory)
  {
    new pqProxyCategory(
      parentCategory, QString(pqInternal::FAVORITES_CATEGORY()), tr("&Favorites"));
  }

  /**
   * Loads settings and force existence of Favorites if enabled.
   */
  void loadSettings(const QString& resourceTagName, bool enableFavorites)
  {
    this->SettingsCategory->loadSettings(resourceTagName);
    if (enableFavorites && !this->SettingsCategory->isEmpty() &&
      !this->hasFavorites(this->SettingsCategory.get()))
    {
      this->createFavorites(this->SettingsCategory.get());
    }
  }

  QList<QPair<QString, QString>> RecentlyUsed;
  // list of favorites. Each pair is {filterGroup, filterPath} where filterPath
  // is the category path to access the favorite: category1;category2;...;filterName
  QList<QPair<QString, QString>> DeprecatedFavorites;
  QSet<QString> ProxyDefinitionGroupToListen;
  QSet<unsigned long> CallBackIDs;
  QWidget Widget;
  QPointer<QAction> SearchAction;
  unsigned long ProxyManagerCallBackId = 0;
  void* LocalActiveSession = nullptr;

  QPointer<QMenu> RecentMenu;
  QPointer<QMenu> DeprecatedFavoritesMenu;
  QPointer<QAction> CategorySeparator;
  QPointer<QMenu> AlphabeticalMenu;
  QPointer<QMenu> MiscMenu;
  QList<QPointer<QMenu>> CategoriesMenus;

  std::unique_ptr<pqProxyCategory> ApplicationCategory;
  std::unique_ptr<pqProxyCategory> SettingsCategory;

  QMap<QString, QPointer<QAction>> CachedActions;

  bool ClientEnvironmentDone = false;
  bool IsWritingSettings = false;

  QTimer NewDefinitionsTimer;
};

//-----------------------------------------------------------------------------
pqProxyGroupMenuManager::pqProxyGroupMenuManager(
  QMenu* mainMenu, const QString& resourceTagName, bool quickLaunchable, bool enableFavorites)
  : Superclass(mainMenu)
  , ResourceTagName(resourceTagName)
  , Enabled(true)
  , EnableFavorites(enableFavorites)
  , Internal(new pqInternal())
  , SupportsQuickLaunch(quickLaunchable)
{
  if (this->EnableFavorites)
  {
    this->Internal->createFavorites(this->Internal->ApplicationCategory.get());
  }

  this->loadCategorySettings();
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QObject::connect(settings, &pqSettings::modified, [&]() { this->loadCategorySettings(); });

  QObject::connect(pqApplicationCore::instance(), &pqApplicationCore::clientEnvironmentDone, this,
    [&]() { this->Internal->ClientEnvironmentDone = true; });

  QObject::connect(pqApplicationCore::instance(), SIGNAL(loadXML(vtkPVXMLElement*)), this,
    SLOT(loadConfiguration(vtkPVXMLElement*)));

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverRemoved(pqServer*)), this, SLOT(removeProxyDefinitionUpdateObservers()));

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(addProxyDefinitionUpdateObservers()));

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), this,
    SLOT(lookForNewDefinitions()));

  // Plugin can be loaded at any point using the delayed load mechanism
  // This ensure that the new symbols will be added in the UI as soon as the Qt loop is running
  this->Internal->NewDefinitionsTimer.setSingleShot(true);
  this->Internal->NewDefinitionsTimer.setInterval(1);
  QObject::connect(&this->Internal->NewDefinitionsTimer, &QTimer::timeout, this,
    &pqProxyGroupMenuManager::lookForNewDefinitions);

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

  this->connect(mainMenu, SIGNAL(aboutToShow()), SLOT(populateCategoriesMenus()));

  this->populateMenu();
}

//-----------------------------------------------------------------------------
pqProxyGroupMenuManager::~pqProxyGroupMenuManager()
{
  this->removeProxyDefinitionUpdateObservers();
  if (vtkSMProxyManager::IsInitialized())
  {
    vtkSMProxyManager::GetProxyManager()->RemoveObserver(this->Internal->ProxyManagerCallBackId);
  }
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::addProxy(const QString& xmlgroup, const QString& xmlname)
{
  if (!xmlname.isEmpty() && !xmlgroup.isEmpty())
  {
    auto proxy =
      new pqProxyInfo(this->Internal->ApplicationCategory.get(), xmlname, xmlgroup, xmlname);
    this->Internal->ApplicationCategory->addProxy(proxy);
  }
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::removeProxy(const QString& xmlgroup, const QString& xmlname)
{
  if (!xmlname.isEmpty() && !xmlgroup.isEmpty())
  {
    this->Internal->ApplicationCategory->removeProxy(xmlname);
  }
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::loadConfiguration(vtkPVXMLElement* root)
{
  if (!root || !root->GetName())
  {
    return;
  }
  if (this->ResourceTagName != root->GetName())
  {
    this->loadConfiguration(root->FindNestedElementByName(this->ResourceTagName.toUtf8().data()));
    return;
  }

  // Convert legacy xml to new style.
  pqProxyCategory::convertLegacyXML(root);

  bool modified = this->Internal->ApplicationCategory->parseXML(root);
  // do not re-add application defined categories inside settings.

  if (modified && this->Internal->allowSettingsUpdate())
  {
    modified = this->Internal->SettingsCategory->parseXML(root);
    if (modified)
    {
      this->writeCategoryToSettings();
    }
  }

  this->populateMenu();
}

//-----------------------------------------------------------------------------
static bool actionTextSort(QAction* a, QAction* b)
{
  return a->text() < b->text();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::populateMiscMenu()
{
  if (!this->Internal->MiscMenu)
  {
    return;
  }

  this->Internal->MiscMenu->clear();

  // get proxies that are under a category.
  QStringList categorizedProxyNames;
  auto categories = this->getMenuCategory()->getSubCategories();
  for (auto category : categories)
  {
    auto categoryProxies = category->getProxiesRecursive();
    for (auto proxy : categoryProxies)
    {
      if (proxy->hideFromMenu())
      {
        continue;
      }
      categorizedProxyNames << proxy->name();
    }
  }

  // add in Misc menu each application-defined proxy that is not under a category.
  auto applicationProxies = this->Internal->ApplicationCategory->getProxiesRecursive();
  for (auto proxy : applicationProxies)
  {
    if (categorizedProxyNames.contains(proxy->name()))
    {
      continue;
    }
    auto action = this->getAction(proxy);
    if (action)
    {
      this->Internal->MiscMenu->addAction(action);
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::populateAlphabeticalMenu()
{
  if (!this->Internal->AlphabeticalMenu)
  {
    return;
  }

  this->Internal->AlphabeticalMenu->clear();

  auto applicationProxies = this->Internal->ApplicationCategory->getProxiesRecursive();
  QList<QAction*> allProxiesActions;

  for (auto proxy : applicationProxies)
  {
    QAction* action = this->getAction(proxy);
    if (action && !allProxiesActions.contains(action))
    {
      allProxiesActions.push_back(action);
    }
  }

  // Now sort all actions added in temp based on their texts.
  std::sort(allProxiesActions.begin(), allProxiesActions.end(), ::actionTextSort);
  for (QAction* action : allProxiesActions)
  {
    this->Internal->AlphabeticalMenu->addAction(action);
  }
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
    QStringList parts = list.split("|", PV_QT_SKIP_EMPTY_PARTS);
    for (const QString& part : parts)
    {
      QStringList pieces = part.split(";", PV_QT_SKIP_EMPTY_PARTS);
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
void pqProxyGroupMenuManager::populateCategoryMenu(QMenu* parentMenu, pqProxyCategory* category)
{
  QList<QAction*> action_list = this->categoryActions(category);

  QMenu* subMenu = new QMenu(category->label(), parentMenu) << pqSetName(category->name());
  parentMenu->insertMenu(this->Internal->MiscMenu->menuAction(), subMenu);

  this->populateSubCategoriesMenus(subMenu, category);
  for (auto action : action_list)
  {
    subMenu->addAction(action);
  }

  this->Internal->CategoriesMenus.append(subMenu);
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::clearCategoriesMenus()
{
  for (auto menu : this->Internal->CategoriesMenus)
  {
    if (menu)
    {
      this->menu()->removeAction(menu->menuAction());
      delete menu;
    }
  }
  this->Internal->CategoriesMenus.clear();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::populateSubCategoriesMenus(QMenu* parent, pqProxyCategory* category)
{
  QList<pqProxyCategory*> sortedCategories = category->getCategoriesAlphabetically();

  for (auto subCategory : sortedCategories)
  {
    this->populateCategoryMenu(parent, subCategory);
  }
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::populateCategoriesMenus()
{
  this->clearCategoriesMenus();

  this->populateSubCategoriesMenus(this->menu(), this->Internal->menuCategory());

  this->updateActionsStyle();

  // ensure "Favorites" is above the category list.
  QMenu* favoritesMenu = this->getFavoritesMenu();
  if (this->EnableFavorites && favoritesMenu)
  {
    this->menu()->insertMenu(this->Internal->CategorySeparator, favoritesMenu);
    auto addToFavorites = this->createAddToFavoritesAction();
    favoritesMenu->addAction(addToFavorites);
  }

  this->populateMiscMenu();

  Q_EMIT this->menuPopulated();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::populateFavoritesMenu()
{
  this->Internal->deprecatedLoadFavoritesItems(this->ResourceTagName);
  this->updateMenuStyle();
  if (!this->Internal->DeprecatedFavoritesMenu)
  {
    return;
  }

  this->Internal->DeprecatedFavoritesMenu->clear();
  this->Internal->DeprecatedFavoritesMenu->menuAction()->setVisible(false);

  QAction* manageFavoritesAction =
    this->Internal->DeprecatedFavoritesMenu->addAction(tr("&Manage Favorites..."))
    << pqSetName("actionManage_Favorites");
  new pqManageFavoritesReaction(manageFavoritesAction, this);

  this->Internal->DeprecatedFavoritesMenu->addAction(
    this->Internal->deprecatedGetAddToFavoritesAction(QString(), this));
  this->Internal->DeprecatedFavoritesMenu->addSeparator();

  if (!this->Internal->DeprecatedFavorites.empty())
  {
    for (const QPair<QString, QString>& key : this->Internal->DeprecatedFavorites)
    {
      QStringList categories = key.second.split(";", PV_QT_SKIP_EMPTY_PARTS);
      bool isCategory = key.first.compare("categories") == 0;
      QString filter = isCategory ? QString("") : categories.takeLast();
      if (!isCategory)
      {
        categories.removeLast();
      }

      QMenu* submenu = this->Internal->DeprecatedFavoritesMenu;
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
          submenu->addAction(this->Internal->deprecatedGetAddToFavoritesAction(path, this));
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
QAction* pqProxyGroupMenuManager::getAddToFavoritesAction(const QString& path)
{
  return this->Internal->deprecatedGetAddToFavoritesAction(path, this);
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::loadFavoritesItems()
{
  this->Internal->deprecatedLoadFavoritesItems(this->ResourceTagName);
  this->updateMenuStyle();
}

//-----------------------------------------------------------------------------
QMenu* pqProxyGroupMenuManager::getFavoritesMenu()
{
  if (this->EnableFavorites)
  {
    for (auto menu : this->Internal->CategoriesMenus)
    {
      if (menu && menu->objectName() == pqInternal::FAVORITES_CATEGORY())
      {
        return menu;
      }
    }
    return this->Internal->DeprecatedFavoritesMenu;
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
QString pqProxyGroupMenuManager::categoryLabel(const QString& category)
{
  pqCategoryMap allCategories = this->Internal->menuCategory()->getSubCategoriesRecursive();
  if (allCategories.contains(category))
  {
    return allCategories[category]->label();
  }

  return QString();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::clearMenu()
{
  // We reuse QAction instances, yet we don't want to have callbacks set up for
  // actions that are no longer shown in the menu. Hence we disconnect all
  // signal connections.
  QMenu* mainMenu = this->menu();

  QList<QAction*> menuActions = mainMenu->actions();
  for (QAction* action : menuActions)
  {
    QObject::disconnect(action, nullptr, this, nullptr);
  }
  menuActions.clear();
  if (!this->Internal->SearchAction.isNull())
  {
    this->Internal->SearchAction->deleteLater();
  }

  QList<QMenu*> submenus = mainMenu->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
  for (QMenu* submenu : submenus)
  {
    delete submenu;
  }
  mainMenu->clear();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::populateMenu()
{
  this->clearMenu();

  QMenu* mainMenu = this->menu();

  if (this->supportsQuickLaunch())
  {
#if defined(Q_WS_MAC) || defined(Q_OS_MAC)
    this->Internal->SearchAction =
      mainMenu->addAction(tr("Search...\tAlt+Space"), this, SLOT(quickLaunch()));
#else
    this->Internal->SearchAction =
      mainMenu->addAction(tr("Search...\tCtrl+Space"), this, SLOT(quickLaunch()));
#endif
    this->Internal->SearchAction->setObjectName("quickLaunchAction");
  }

  if (this->RecentlyUsedMenuSize > 0)
  {
    auto* rmenu = mainMenu->addMenu(tr("&Recent")) << pqSetName("Recent");
    this->Internal->RecentMenu = rmenu;
    this->connect(rmenu, SIGNAL(aboutToShow()), SLOT(populateRecentlyUsedMenu()));
  }

  if (this->EnableFavorites)
  {
    auto* bmenu = mainMenu->addMenu(tr("&OldFavorites")) << pqSetName("LegacyFavorites");
    bmenu->menuAction()->setVisible(false);
    this->Internal->DeprecatedFavoritesMenu = bmenu;
  }

  this->Internal->CategorySeparator = mainMenu->addSeparator();

  // Add alphabetical list.
  this->Internal->AlphabeticalMenu = mainMenu->addMenu(tr("&Alphabetical"))
    << pqSetName("Alphabetical");
  this->populateAlphabeticalMenu();

  this->Internal->MiscMenu = mainMenu->addMenu(tr("&Miscellaneous")) << pqSetName("Miscellaneous");

  mainMenu->addSeparator();

  this->updateActionsStyle();

  Q_EMIT this->menuPopulated();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::updateActionsStyle()
{
  for (QAction* action : this->Internal->CachedActions)
  {
    QFont font = action->font();
    font.setBold(false);

    vtkSMProxy* prototype = pqProxyAction::GetProxyPrototype(action);
    if (prototype)
    {
      font.setItalic(prototype->IsDeprecated());
    }

    action->setFont(font);
  }

  auto favoritesCategory = this->getFavoritesCategory();
  if (!favoritesCategory)
  {
    return;
  }

  auto favorites = favoritesCategory->getProxiesRecursive();
  for (auto favorite : favorites)
  {
    auto action = this->getAction(favorite);
    if (action && !favorite->hideFromMenu())
    {
      QFont font = action->font();
      font.setBold(true);
      action->setFont(font);
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::updateMenuStyle()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  bool sc = settings->value("GeneralSettings.ForceSingleColumnMenus", false).toBool();
  this->menu()->setStyleSheet(QString("QMenu { menu-scrollable: %1; }").arg(sc ? 1 : 0));

  this->updateActionsStyle();
}

//-----------------------------------------------------------------------------
QAction* pqProxyGroupMenuManager::getAction(pqProxyInfo* proxyInfo)
{
  // look in cache for non null action.
  if (this->Internal->CachedActions.contains(proxyInfo->name()))
  {
    auto action = this->Internal->CachedActions[proxyInfo->name()];
    this->Internal->updateAction(action, proxyInfo);
    return action;
  }

  return this->createAction(proxyInfo);
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::updateActionIcon(pqProxyInfo* proxy)
{
  auto action = this->getAction(proxy);
  if (action)
  {
    this->Internal->updateActionIcon(action, proxy);
  }
}

//-----------------------------------------------------------------------------
QAction* pqProxyGroupMenuManager::createAction(pqProxyInfo* proxyInfo)
{
  const QString& group = proxyInfo->group();
  const QString& name = proxyInfo->name();

  auto action = new QAction(this);

  QStringList data_list;
  data_list << group << name;
  action << pqSetName(name) << pqSetData(data_list);
  action->setText(proxyInfo->label());

  vtkSMProxy* prototype = pqProxyAction::GetProxyPrototype(action);
  // create action only for valid proxies
  if (!prototype)
  {
    action->deleteLater();
    return nullptr;
  }

  QString tooltip = pqProxyAction::GetProxyDocumentation(action);
  action->setToolTip(tooltip);

  // Add action in the pool for the QuickSearch...
  this->Internal->Widget.addAction(action);
  this->Internal->CachedActions[proxyInfo->name()] = action;
  this->Internal->updateAction(action, proxyInfo);

  // this avoids creating duplicate connections.
  this->connect(action, SIGNAL(triggered()), SLOT(triggered()), Qt::UniqueConnection);

  return action;
}

//-----------------------------------------------------------------------------
QAction* pqProxyGroupMenuManager::getAction(const QString& pgroup, const QString& pname)
{
  if (pname.isEmpty() || pgroup.isEmpty())
  {
    vtkGenericWarningMacro("Cannot find action for proxy, no name or group.");
    return nullptr;
  }

  if (this->Internal->CachedActions.contains(pname))
  {
    return this->Internal->CachedActions[pname];
  }

  auto proxyList = this->Internal->ApplicationCategory->getProxiesRecursive();
  for (auto proxy : proxyList)
  {
    if (proxy->name() == pname && proxy->group() == pgroup)
    {
      return this->createAction(proxy);
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
QAction* pqProxyGroupMenuManager::createAddToFavoritesAction()
{
  QString actionName = QString("actionAddToFavorites");
  auto action = new QAction(tr("Add to Favorites"), this) << pqSetName(actionName);
  new pqAddToFavoritesReaction(action, this);
  return action;
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
  Q_EMIT this->triggered(key.first, key.second);
  if (this->RecentlyUsedMenuSize > 0)
  {
    this->Internal->RecentlyUsed.removeAll(key);
    this->Internal->RecentlyUsed.push_front(key);
    while (this->Internal->RecentlyUsed.size() > this->RecentlyUsedMenuSize)
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
vtkSMProxy* pqProxyGroupMenuManager::getPrototype(QAction* action) const
{
  return pqProxyAction::GetProxyPrototype(action);
}

//-----------------------------------------------------------------------------
QString pqProxyGroupMenuManager::getToolbarName(pqProxyCategory* category)
{
  // get group from proxies, but default to ResourceTagName
  QString groupName = this->ResourceTagName;
  auto proxies = category->getRootProxies();
  if (!proxies.empty())
  {
    auto someProxy = proxies[0];
    if (someProxy)
    {
      groupName = someProxy->group();
    }
  }

  auto toolbarName = QString("%1.%2").arg(groupName).arg(category->name());

  return toolbarName;
}

//-----------------------------------------------------------------------------
QStringList pqProxyGroupMenuManager::getToolbarCategories() const
{
  QStringList categories_in_toolbar;

  auto categories = this->Internal->menuCategory()->getSubCategoriesRecursive();
  for (auto category : categories)
  {
    if (category->showInToolbar())
    {
      categories_in_toolbar.push_back(category->name());
    }
  }
  return categories_in_toolbar;
}

//-----------------------------------------------------------------------------
QList<QAction*> pqProxyGroupMenuManager::categoryActions(const QString& categoryName)
{
  QList<QAction*> category_actions;
  auto categories = this->Internal->menuCategory()->getSubCategoriesRecursive();
  if (!categories.contains(categoryName))
  {
    return category_actions;
  }

  return this->categoryActions(categories[categoryName]);
}

//-----------------------------------------------------------------------------
QList<QAction*> pqProxyGroupMenuManager::categoryActions(pqProxyCategory* category)
{
  QList<QAction*> category_actions;
  if (category->isEmpty())
  {
    return category_actions;
  }

  auto proxies = category->getRootProxies();
  QStringList proxiesOriginalOrdering = category->getOrderedRootProxiesNames();

  for (auto proxyName : proxiesOriginalOrdering)
  {
    auto proxy = category->findProxy(proxyName);
    QAction* action = this->getAction(proxy);
    if (action && !proxy->hideFromMenu())
    {
      category_actions.push_back(action);
    }
  }

  // alphabetical sort unless the XML overrode the sorting using the "preserve_order"
  // attribute. (see #8364)
  if (!category->preserveOrder())
  {
    std::sort(category_actions.begin(), category_actions.end(), ::actionTextSort);
  }

  return category_actions;
}

//-----------------------------------------------------------------------------
QList<QAction*> pqProxyGroupMenuManager::actionsInToolbars()
{
  auto categories = this->getToolbarCategories();
  QList<QAction*> actions_in_toolbars;

  for (const auto& categoryName : categories)
  {
    auto categoryActions = this->categoryActions(categoryName);

    for (auto action : categoryActions)
    {
      QVariant omitFromToolbar = action->property("OmitFromToolbar");
      if (!omitFromToolbar.isValid() || !omitFromToolbar.toStringList().contains(categoryName))
      {
        actions_in_toolbars.push_back(action);
      }
    }
  }

  return actions_in_toolbars;
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
  for (unsigned long callbackID : this->Internal->CallBackIDs)
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
    &this->Internal->NewDefinitionsTimer, &QTimer::start);
  this->Internal->CallBackIDs.insert(callbackID);

  // compound proxy
  callbackID = pxm->AddObserver(vtkSMProxyDefinitionManager::CompoundProxyDefinitionsUpdated,
    &this->Internal->NewDefinitionsTimer, &QTimer::start);
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

  if (this->Internal->ProxyDefinitionGroupToListen.empty() || pxm == nullptr)
  {
    return; // Nothing to look into...
  }
  vtkSMProxyDefinitionManager* pxdm = pxm->GetProxyDefinitionManager();

  // Setup definition iterator
  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pxdm->NewIterator());
  for (const QString& groupName : this->Internal->ProxyDefinitionGroupToListen)
  {
    iter->AddTraversalGroupName(groupName.toUtf8().data());
  }

  // Loop over proxy that should be inserted inside the UI
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    const char* group = iter->GetGroupName();
    const char* name = iter->GetProxyName();
    vtkPVXMLElement* hints = iter->GetProxyHints();
    if (!hints || hints->FindNestedElementByName("ReaderFactory") != nullptr)
    {
      // skip readers.
      continue;
    }

    bool modified = this->Internal->ApplicationCategory->parseXMLHintsTag(group, name, hints);
    // do not re-add application defined categories inside settings.
    if (modified && this->Internal->allowSettingsUpdate())
    {
      modified = this->Internal->SettingsCategory->parseXMLHintsTag(group, name, hints);
      if (modified)
      {
        this->writeCategoryToSettings();
      }
    }
  }

  this->populateMenu();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::switchActiveServer()
{
  void* newActiveSession = vtkSMProxyManager::IsInitialized()
    ? vtkSMProxyManager::GetProxyManager()->GetActiveSession()
    : nullptr;

  if (newActiveSession && newActiveSession != this->Internal->LocalActiveSession)
  {
    // Make sure we don't clear the menu twice for the same server
    this->Internal->LocalActiveSession = newActiveSession;

    // Clear the QuickSearch QAction pool...
    QList<QAction*> action_list = this->Internal->Widget.actions();
    for (QAction* action : action_list)
    {
      this->Internal->Widget.removeAction(action);
      delete action;
    }
    this->Internal->CachedActions.clear();

    // Fill it back by updating the menu
    this->lookForNewDefinitions();
  }
}

//-----------------------------------------------------------------------------
pqProxyCategory* pqProxyGroupMenuManager::getApplicationCategory()
{
  return this->Internal->ApplicationCategory.get();
}

//-----------------------------------------------------------------------------
pqProxyCategory* pqProxyGroupMenuManager::getMenuCategory()
{
  return this->Internal->menuCategory();
}

//-----------------------------------------------------------------------------
pqProxyCategory* pqProxyGroupMenuManager::getFavoritesCategory()
{
  return this->Internal->menuCategory()->findSubCategory(pqInternal::FAVORITES_CATEGORY());
}

//-----------------------------------------------------------------------------
bool pqProxyGroupMenuManager::isFavorites(pqProxyCategory* category)
{
  return category && category->name() == pqInternal::FAVORITES_CATEGORY();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::loadCategorySettings()
{
  if (this->Internal->IsWritingSettings)
  {
    return;
  }

  this->Internal->loadSettings(this->ResourceTagName, this->EnableFavorites);

  Q_EMIT this->categoriesUpdated();
}

//-----------------------------------------------------------------------------
void pqProxyGroupMenuManager::writeCategoryToSettings()
{
  bool prev = this->Internal->IsWritingSettings;
  this->Internal->IsWritingSettings = true;
  this->getMenuCategory()->writeSettings(this->ResourceTagName);
  this->Internal->IsWritingSettings = prev;
}
