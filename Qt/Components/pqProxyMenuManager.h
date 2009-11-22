/*=========================================================================

   Program: ParaView
   Module:    pqProxyMenuManager.h

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

========================================================================*/
#ifndef __pqProxyMenuManager_h 
#define __pqProxyMenuManager_h

#include <QObject>
#include "pqComponentsExport.h"

class QMenu;
class QAction;

// Keeps a menu updated using the proxy definitions under a particular group.
// Useful for sources/filters menus.
// OBSOLETE - TO DEPRECATE (this and subclasses).
class PQCOMPONENTS_EXPORT pqProxyMenuManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqProxyMenuManager(QMenu* menu);
  virtual ~pqProxyMenuManager();
  QMenu* menu()
    { return this->Menu;}

  /// Set the name of the server manager group. The menu lists the proxy
  /// definitions available under this group.
  void setXMLGroup(const QString& group)
    { this->XMLGroup = group; }

  /// Returns the XML group set using setXMLGroup.
  const QString& xmlGroup() const
    { return this->XMLGroup; }

  /// Set the name of the directory which contains the XML files used for
  /// filtering. When initialize() is called, only those proxies present in the
  /// filtering XML files will be added to the menu. This can be a resource
  /// directory name as well.
  void setFilteringXMLDir(const QString& str)
    { this->FilteringXMLDir = str; }

  /// Returns the directory name set using setFilteringXMLDir().
  const QString& filteringXMLDir() const
    { return this->FilteringXMLDir; }

  /// Set the tag name for the element used in the filtering XML. eg. for
  /// filters we use the <Filter /> tag, while for sources we use the <Source />
  /// tag. 
  void setElementTagName(const QString& tag)
    { this->ElementTagName = tag; }
  const QString& elementTagName() const
    { return this->ElementTagName; }

  /// When size>0 a recently used category will be added to the menu.
  /// One must call update() or initialize() after changing this value.
  void setRecentlyUsedMenuSize(unsigned int val)
    { this->RecentlyUsedMenuSize = val; }

  unsigned int recentlyUsedMenuSize() const
    { return this->RecentlyUsedMenuSize; }

public slots:
  /// Initializes the menu using the current proxy manager state. Only those
  /// proxies that are specified by the filtering XML will be shown in the menu.
  void initialize();

  /// Update the contents of the menu using the current proxy manager state.
  /// Any new proxy definitions that may have become available since the last
  /// time update() or initialize() was called, will automatically get added to
  /// the menu (irrespective of whether the new proxy is made accessible by the
  /// filtering XML).
  void update();

protected:
  /// overloadable filter to return whether a certain proxy
  /// be included in the menu
  virtual bool filter(const QString& /*name*/) { return true; }

signals:
  /// Fired when a item in the menu is selected.
  void selected(const QString& name);

  /// Fired each time the menu is rebuilt.
  void menuPopulated();

private slots:
  /// Called when an action on the menu is triggered.
  void triggered();

private:
  QMenu* Menu;
  QString XMLGroup;
  QString FilteringXMLDir;
  QString ElementTagName;
  unsigned int RecentlyUsedMenuSize;

  /// Populates the menu.
  void populateMenu();

  /// Returns the action for a given proxy.
  QAction* getAction(const QString& proxyname);

  void updateFromXML();
  void updateFromXML(const QString& xmlfilename);

  void loadRecentlyUsedItems();
  void saveRecentlyUsedItems();

  void populateRecentlyUsedMenu(QMenu*);
  class pqInternal;
  pqInternal* Internal;
private:
  pqProxyMenuManager(const pqProxyMenuManager&); // Not implemented.
  void operator=(const pqProxyMenuManager&); // Not implemented.
};

#endif


