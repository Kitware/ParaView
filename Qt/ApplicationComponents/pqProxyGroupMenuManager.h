// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxyGroupMenuManager_h
#define pqProxyGroupMenuManager_h

#include "pqApplicationComponentsModule.h"

#include <QMenu>

#include "vtkParaViewDeprecation.h" // for deprecation macro

#include <memory> // for unique_ptr

class pqProxyCategory;
class pqProxyInfo;
class vtkPVXMLElement;
class vtkSMProxy;

/**
 * pqProxyGroupMenuManager is a menu-populator that fills up a menu with
 * proxies defined in an XML configuration file. This is use to automatically
 * build the sources and filters menu in ParaView.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqProxyGroupMenuManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
   * Constructor.
   * \c menu is the Menu to be populated.
   * \c resourceTagName is the tag name eg. "ParaViewSources" in the client
   *    configuration files which contains lists the items shown by this menu.
   * \c supportsQuickLaunch, set to false if quick-launch is not to be supported
   *    for this menu.
   */
  pqProxyGroupMenuManager(QMenu* menu, const QString& resourceTagName,
    bool supportsQuickLaunch = true, bool enableFavorites = false);
  ~pqProxyGroupMenuManager() override;

  /**
   * Access the menu.
   */
  QMenu* menu() const { return static_cast<QMenu*>(this->parent()); }

  /**
   * returns the widget that hold actions created by this menu manager.
   */
  QWidget* widgetActionsHolder() const;
  QMenu* getFavoritesMenu();

  /**
   * When size>0 a recently used category will be added to the menu.
   * One must call update() or initialize() after changing this value.
   */
  void setRecentlyUsedMenuSize(unsigned int val) { this->RecentlyUsedMenuSize = val; }
  unsigned int recentlyUsedMenuSize() const { return this->RecentlyUsedMenuSize; }
  /**
   * Returns true if the pqProxyGroupMenuManager has been registered with
   * quick-launch mechanism maintained by pqApplicationCore.
   */
  bool supportsQuickLaunch() const { return this->SupportsQuickLaunch; }
  void setEnableFavorites(bool enable) { this->EnableFavorites = enable; }

  /**
   * Returns the prototype proxy for the action.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("Use pqProxyAction::GetProxyPrototype instead.")
  vtkSMProxy* getPrototype(QAction* action) const;

  /**
   * Provides mechanism to explicitly add a proxy to the menu.
   */
  void addProxy(const QString& xmlgroup, const QString& xmlname);

  /**
   * Provides mechanism to explicitly remove a proxy to the menu.
   */
  void removeProxy(const QString& xmlgroup, const QString& xmlname);

  /**
   * Categories information.
   */
  ///@{
  /**
   * Returns a list of categories that have the "show_in_toolbar" attribute set
   * to 1.
   */
  QStringList getToolbarCategories() const;
  /**
   * Return a name for the toolbar. This is based on category name and proxy group.
   */
  QString getToolbarName(pqProxyCategory* category);
  /**
   * Given a category name, return the category label.
   */
  QString categoryLabel(const QString& category);
  /**
   * Returns whether or not the category's toolbar should be hidden initially.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0(
    "This was mostly unused. Also it is better to avoid test-dedicated code paths.")
  bool hideForTests(const QString&) const { return false; };
  ///@}

  /**
   * List actions.
   */
  ///@{
  /**
   * returns the actions holds by the widgetActionsHolder
   */
  QList<QAction*> actions() const;
  /**
   * Returns actions from given category. Create actions as needed.
   * Looks for category in the default categories list.
   */
  QList<QAction*> categoryActions(const QString& category);
  /**
   * Returns actions from given category. Create actions as needed.
   */
  QList<QAction*> categoryActions(pqProxyCategory* category);
  /**
   * Returns this list of actions that appear in toolbars
   */
  QList<QAction*> actionsInToolbars();
  /**
   * Return the action for a given proxy.
   */
  QAction* getAction(pqProxyInfo* proxy);
  /**
   * Update action icon from proxy info.
   */
  void updateActionIcon(pqProxyInfo* proxy);
  ///@}

  /**
   * Attach an observer to proxy manager to monitor any proxy definition update
   * The detected proxy have to own a hint:
   * \code
   *     <ShowInMenu category="" icon=""/>
   * \endcode
   * where all the attribute are fully optional.
   */
  void addProxyDefinitionUpdateListener(const QString& proxyGroupName);
  void removeProxyDefinitionUpdateListener(const QString& proxyGroupName);

  /**
   * Specific-meaning categories.
   */
  ///@{
  /**
   * Returns the invisible root category containing all
   * categories to display, as defined by the application.
   */
  pqProxyCategory* getApplicationCategory();
  /**
   * Returns the invisible root category containing all
   * categories to display. Use settings if existing.
   */
  pqProxyCategory* getMenuCategory();
  /**
   * Returns the Favorites category.
   */
  pqProxyCategory* getFavoritesCategory();
  /**
   * Returns true if the given category is a favorites.
   * Based on name.
   */
  bool isFavorites(pqProxyCategory* category);
  ///@}

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Update from servermanager.
   */
  ///@{
  /**
   * Load a configuration XML. It will find the elements with resourceTagName
   * in the XML and populate the menu accordingly. Applications do not need to
   * call this method directly, it's by default connected to
   * pqApplicationCore::loadXML()
   */
  void loadConfiguration(vtkPVXMLElement*);
  /**
   * Look for new proxy definition to add inside the menu
   * Iter over known proxy definitions to parse Hints tag.
   */
  void lookForNewDefinitions();
  /**
   * Remove all ProxyDefinitionUpdate observers to active server
   */
  void removeProxyDefinitionUpdateObservers();
  /**
   * Update the list of ProxyDefinitionUpdate observers to  server
   */
  void addProxyDefinitionUpdateObservers();
  ///@}

  PARAVIEW_DEPRECATED_IN_5_13_0("Inner member is not used")
  void setEnabled(bool){};

  /**
   * Forces a re-population of the menu.
   * Create the main submenu entries. Actions are mostly created on "aboutToShow" signal,
   * as menu should present the currently available proxies.
   * See also populateRecentlyUsedMenu, populateAlphabeticalMenu, populateMiscMenu,
   * populateCategoriesMenus, clearMenu
   */
  virtual void populateMenu();

  /**
   * Write categories to settings.
   */
  void writeCategoryToSettings();

Q_SIGNALS:
  void triggered(const QString& group, const QString& name);

  /**
   * fired when the menu gets repopulated,typically means that the actions have
   * been updated.
   */
  void menuPopulated();

  /**
   * Fired when categories were updated.
   */
  void categoriesUpdated();

protected Q_SLOTS:
  void triggered();
  void quickLaunch();
  void switchActiveServer();
  void updateMenuStyle();
  void updateActionsStyle();

  /// Fill recently used submenu
  void populateRecentlyUsedMenu();

  /**
   * called when "favorites" menu is being shown.
   * create the menu (and submenu) with actions for the filters in the favorites list.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0(
    "Favorites are now a specific category, configurable as the others.")
  void populateFavoritesMenu();

protected: // NOLINT(readability-redundant-access-specifiers)
  QString ResourceTagName;
  vtkPVXMLElement* MenuRoot = nullptr;
  int RecentlyUsedMenuSize = 0;
  PARAVIEW_DEPRECATED_IN_5_13_0("Inner member is not used")
  bool Enabled;
  bool EnableFavorites = false;

  void loadRecentlyUsedItems();
  void saveRecentlyUsedItems();

  /**
   * Load the favorites from settings.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0(
    "Favorites are now a specific category, configurable as the others.")
  void loadFavoritesItems();

  PARAVIEW_DEPRECATED_IN_5_13_0(
    "Favorites are now a specific category, configurable as the others.")
  QAction* getAddToFavoritesAction(const QString& path);

  /**
   * Return the action for a given proxy.
   */
  QAction* getAction(const QString& pgroup, const QString& proxyname);

private Q_SLOTS:
  /**
   * Populate submenus with current actions.
   * See also populateMenu
   */
  ///@{
  /// Fill alphabetical submenu
  void populateAlphabeticalMenu();
  /// Fill miscellaneous submenu with proxies that are not part of any category.
  void populateMiscMenu();
  /// Append categories to the end of the main menu.
  void populateCategoriesMenus();
  ///@}

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqProxyGroupMenuManager)

  /**
   * Create the action for a given proxy.
   * Return nullptr if its prototype is not known by the proxy manager.
   */
  QAction* createAction(pqProxyInfo* proxy);

  /**
   * Create the action and the reaction to add to favorites under given category.
   */
  QAction* createAddToFavoritesAction();

  /**
   * Updating menus content.
   */
  ///@{
  /// Clear main menu.
  void clearMenu();
  /// Remove all categories menus.
  void clearCategoriesMenus();
  /// populate given menu with categories.
  void populateSubCategoriesMenus(QMenu* parent, pqProxyCategory* category);
  /// populate category menu with proxies and sub categories.
  void populateCategoryMenu(QMenu* parent, pqProxyCategory* category);
  ///@}

  /**
   * Load categories information from settings.
   */
  void loadCategorySettings();

  struct pqInternal;
  std::unique_ptr<pqInternal> Internal;
  bool SupportsQuickLaunch = true;
};

#endif
