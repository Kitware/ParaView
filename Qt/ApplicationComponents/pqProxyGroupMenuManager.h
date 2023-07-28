// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxyGroupMenuManager_h
#define pqProxyGroupMenuManager_h

#include "pqApplicationComponentsModule.h"
#include <QMenu>

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
  pqProxyGroupMenuManager(
    QMenu* menu, const QString& resourceTagName, bool supportsQuickLaunch = true);
  ~pqProxyGroupMenuManager() override;

  /**
   * Access the menu.
   */
  QMenu* menu() const { return static_cast<QMenu*>(this->parent()); }

  /**
   * When size>0 a recently used category will be added to the menu.
   * One must call update() or initialize() after changing this value.
   */
  void setRecentlyUsedMenuSize(unsigned int val) { this->RecentlyUsedMenuSize = val; }

  unsigned int recentlyUsedMenuSize() const { return this->RecentlyUsedMenuSize; }

  /**
   * returns the widget that hold actions created by this menu manager.
   */
  QWidget* widgetActionsHolder() const;

  /**
   * returns the actions holds by the widgetActionsHolder
   */
  QList<QAction*> actions() const;

  /**
   * Returns the prototype proxy for the action.
   */
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
   * Returns a list of categories that have the "show_in_toolbar" attribute set
   * to 1.
   */
  QStringList getToolbarCategories() const;

  /**
   * Returns the list of actions in a category.
   */
  QList<QAction*> actions(const QString& category);

  /**
   * Returns this list of actions that appear in toolbars
   */
  QList<QAction*> actionsInToolbars();

  /**
   * Returns whether or not the category's toolbar should be hidden initially.
   */
  bool hideForTests(const QString& category) const;

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
   * Returns true if the pqProxyGroupMenuManager has been registered with
   * quick-launch mechanism maintained by pqApplicationCore.
   */
  bool supportsQuickLaunch() const { return this->SupportsQuickLaunch; }

  void setEnableFavorites(bool enable) { this->EnableFavorites = enable; }

  QMenu* getFavoritesMenu();

  /**
   * Given a category name, return the category label.
   */
  QString categoryLabel(const QString& category);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Load a configuration XML. It will find the elements with resourceTagName
   * in the XML and populate the menu accordingly. Applications do not need to
   * call this method directly, it's by default connected to
   * pqApplicationCore::loadXML()
   */
  void loadConfiguration(vtkPVXMLElement*);

  /**
   * Look for new proxy definition to add inside the menu
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

  /**
   * Enable/disable the menu and the actions.
   */
  void setEnabled(bool enable);

  /**
   * Forces a re-population of the menu. Any need to call this only after
   * addProxy() has been used to explicitly add entries.
   */
  virtual void populateMenu();

Q_SIGNALS:
  void triggered(const QString& group, const QString& name);

  /**
   * fired when the menu gets repopulated,typically means that the actions have
   * been updated.
   */
  void menuPopulated();

protected Q_SLOTS:
  void triggered();
  void quickLaunch();
  void switchActiveServer();
  void updateMenuStyle();

  /**
   * called when "recent" menu is being shown.
   * updates the menu with actions for the filters in the recent list.
   */
  void populateRecentlyUsedMenu();

  /**
   * called when "favorites" menu is being shown.
   * create the menu (and submenu) with actions for the filters in the favorites list.
   */
  void populateFavoritesMenu();

protected: // NOLINT(readability-redundant-access-specifiers)
  QString ResourceTagName;
  vtkPVXMLElement* MenuRoot;
  int RecentlyUsedMenuSize;
  bool Enabled;
  bool EnableFavorites;

  void loadRecentlyUsedItems();
  void saveRecentlyUsedItems();

  /**
   * Load the favorites from settings.
   */
  void loadFavoritesItems();

  /**
   * Returns the action for a given proxy.
   */
  QAction* getAction(const QString& pgroup, const QString& proxyname);

  QAction* getAddToCategoryAction(const QString& path);

private:
  Q_DISABLE_COPY(pqProxyGroupMenuManager)

  class pqInternal;
  pqInternal* Internal;
  bool SupportsQuickLaunch;
};

#endif
