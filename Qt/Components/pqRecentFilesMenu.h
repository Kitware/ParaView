/*=========================================================================

   Program: ParaView
   Module:    pqRecentFilesMenu.h

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
  pqRecentFilesMenu(QMenu& menu, QObject* p = 0);
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

private:
  pqRecentFilesMenu(const pqRecentFilesMenu&);
  pqRecentFilesMenu& operator=(const pqRecentFilesMenu&);

  QPointer<QMenu> Menu;
  bool SortByServers;
};

#endif // !_pqRecentFilesMenu_h
