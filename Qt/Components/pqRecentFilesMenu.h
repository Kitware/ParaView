// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRecentFilesMenu_h
#define pqRecentFilesMenu_h

#include <QObject>

#include "pqComponentsModule.h" // needed for export macros
#include <QPointer>             // needed for QPointer

class pqServer;
class pqServerResource;
class QAction;
class QMenu;

/**
 * @class pqRecentFilesMenu
 * @brief manages recent files menu used in ParaView.
 *
 * pqRecentFilesMenu manages the recent files (states, etc.) menu used in
 * ParaView. pqRecentFilesMenu uses implementations of
 * `pqRecentlyUsedResourceLoaderInterface` to handle how to show (icon, label) a recent item
 * and how to handle user requesting to load that item.
 *
 * To add support for different types of recent items in your custom
 * application, you may want to provide new implementations for
 * `pqRecentlyUsedResourceLoaderInterface`.
 *
 * @sa pqStandardRecentlyUsedResourceLoaderImplementation
 * @sa pqParaViewBehaviors,
 * @sa pqParaViewMenuBuilders
 */

class PQCOMPONENTS_EXPORT pqRecentFilesMenu : public QObject
{
  Q_OBJECT

public:
  /**
   * Assigns the menu that will display the list of files
   */
  pqRecentFilesMenu(QMenu& menu, QObject* p = nullptr);
  ~pqRecentFilesMenu() override;

  /**
   * Open a resource on the given server
   */
  virtual bool open(pqServer* server, const pqServerResource& resource) const;

  /**
   * When set to true (default), the menu is arranged to keep resources that use
   * the same server together. This also results in the menu have separators for
   * the server names. Set to false if your application does not support
   * connecting/disconnecting from server or you don't case about sorting the
   * menu by servers.
   */
  void setSortByServers(bool val) { this->SortByServers = val; }
  bool sortByServers() const { return this->SortByServers; }

private Q_SLOTS:
  void buildMenu();
  void onOpenResource(QAction*);
  void onOpenResource(const pqServerResource& resource);

private: // NOLINT(readability-redundant-access-specifiers)
  pqRecentFilesMenu(const pqRecentFilesMenu&);
  pqRecentFilesMenu& operator=(const pqRecentFilesMenu&);

  QPointer<QMenu> Menu;
  bool SortByServers;
};

#endif // !pqRecentFilesMenu_h
