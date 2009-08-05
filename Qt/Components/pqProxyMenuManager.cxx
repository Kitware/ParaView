/*=========================================================================

   Program: ParaView
   Module:    pqProxyMenuManager.cxx

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
#include "pqProxyMenuManager.h"

// Server Manager Includes.
#include "vtkSmartPointer.h"
#include "vtkPVXMLParser.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionIterator.h"

// Qt Includes.
#include <QList>
#include <QMap>
#include <QMenu>
#include <QSet>
#include <QtDebug>
#include <QDir>
#include <QPointer>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqSetData.h"
#include "pqSetName.h"
#include "pqSettings.h"

class pqProxyMenuManager::pqInternal
{
public:
  struct Info
    {
    QString Icon; //<-- Name of the icon to use, if any.
    QPointer<QAction> Action; //<-- Action for this proxy.
    };

  typedef QMap<QString, Info> ProxyInfoMap;

  struct CategoryInfo
    {
    QString Label;
    bool PreserveOrder;
    QList<QString> Proxies;
    CategoryInfo() { this->PreserveOrder = false; }
    };

  typedef QMap<QString, CategoryInfo> CategoryInfoMap;
 
  void addProxy(const QString& name, const QString& icon)
    {
    if (!name.isEmpty())
      {
      Info& info = this->Proxies[name];
      if (!icon.isEmpty())
        {
        info.Icon = icon;
        }
      }
    }
 
  // Proxies and Categories is what gets shown in the menu.
  ProxyInfoMap Proxies;
  CategoryInfoMap Categories;

  // Definitions is used to determine which new definitions were added since
  // last time we updated.
  QSet<QString> Definitions;

  QStringList RecentlyUsed;
};

//-----------------------------------------------------------------------------
pqProxyMenuManager::pqProxyMenuManager(QMenu* _menu) :Superclass(_menu)
{
  this->Menu = _menu;
  this->RecentlyUsedMenuSize = 0;
  this->Internal = new pqInternal();
}

//-----------------------------------------------------------------------------
pqProxyMenuManager::~pqProxyMenuManager()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqProxyMenuManager::triggered()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if (!action)
    {
    return;
    }
  QString pname = action->data().toString();
  if (!pname.isEmpty())
    {
    emit this->selected(action->data().toString());

    if (this->RecentlyUsedMenuSize > 0)
      {
      this->Internal->RecentlyUsed.removeAll(pname);
      this->Internal->RecentlyUsed.push_front(pname);
      while (this->Internal->RecentlyUsed.size() > 
        static_cast<int>(this->RecentlyUsedMenuSize))
        {
        this->Internal->RecentlyUsed.pop_back();
        }
      this->populateRecentlyUsedMenu(0);
      this->saveRecentlyUsedItems();
      }
    }
}

//-----------------------------------------------------------------------------
void pqProxyMenuManager::initialize()
{
  if (this->FilteringXMLDir.isEmpty())
    {
    qCritical() << "FilteringXMLDir must be set.";
    return;
    }

  if (this->XMLGroup.isEmpty())
    {
    qCritical() << "XMLGroup must be set.";
    return;
    }

  if (this->ElementTagName.isEmpty())
    {
    qCritical() << "Subclass must set ElementTagName.";
    return;
    }

  this->Internal->Proxies.clear();
  this->Internal->Categories.clear();

  // Process the filtering XML and build up the initial list of proxies.
  this->updateFromXML(); 


  // Fill Definitions with the names of all proxy definitions currently known to
  // the ServerManager. This is used to determine which new proxy definitions
  // got added when update() is called.
  this->Internal->Definitions.clear();
  vtkSMProxyDefinitionIterator* defnIter = vtkSMProxyDefinitionIterator::New();
  defnIter->SetModeToOneGroup();
  for (defnIter->Begin(this->XMLGroup.toAscii().data()); !defnIter->IsAtEnd();
    defnIter->Next())
    {
    if (defnIter->IsCustom())
      {
      if(this->filter(defnIter->GetKey()))
        {
        this->Internal->addProxy(defnIter->GetKey(), NULL);
        }
      }
    this->Internal->Definitions.insert(defnIter->GetKey());
    }
  defnIter->Delete();


  // Update the QMenu.
  this->populateMenu();
}

//-----------------------------------------------------------------------------
void pqProxyMenuManager::update()
{
  if (this->XMLGroup.isEmpty())
    {
    qCritical() << "XMLGroup must be set.";
    return;
    }
  
  // add new proxies from new xml files
  this->updateFromXML(); 

  // Locate new proxy definitions that got added.
  QSet<QString> newDefns;
  vtkSMProxyDefinitionIterator* defnIter = vtkSMProxyDefinitionIterator::New();
  defnIter->SetModeToOneGroup();
  for (defnIter->Begin(this->XMLGroup.toAscii().data()); !defnIter->IsAtEnd();
    defnIter->Next())
    {
    if (!this->Internal->Definitions.contains(defnIter->GetKey()))
      {
      if(this->filter(defnIter->GetKey()))
        {
        newDefns.insert(defnIter->GetKey());
        this->Internal->addProxy(defnIter->GetKey(), /*icon=*/NULL);
        }
      }
    }
  defnIter->Delete();
  this->Internal->Definitions += newDefns;

  this->populateMenu();
}

static bool actionTextSort(QAction* a, QAction *b)
{
  return a->text() < b->text();
}

//-----------------------------------------------------------------------------
void pqProxyMenuManager::populateRecentlyUsedMenu(QMenu* rmenu)
{
  QMenu* recentMenu = rmenu? rmenu : this->Menu->findChild<QMenu*>("Recent");
  if (recentMenu)
    {
    recentMenu->clear();
    foreach (QString pname, this->Internal->RecentlyUsed)
      {
      QAction* action = this->getAction(pname);
      if (action)
        {
        recentMenu->addAction(action);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqProxyMenuManager::populateMenu()
{
  // We reuse QAction instances, yet we don't want to have callbacks set up for
  // actions that are no longer shown in the menu. Hence we disconnect all
  // signal connections.
  QList<QAction*> menuActions = this->Menu->actions();
  foreach (QAction* action, menuActions)
    {
    QObject::disconnect(action, 0,  this, 0);
    }
  menuActions.clear();

  QList<QMenu*> submenus = this->Menu->findChildren<QMenu*>();
  foreach (QMenu* submenu, submenus)
    {
    delete submenu;
    }
  this->Menu->clear();

  if (this->RecentlyUsedMenuSize > 0)
    {
    QMenu* recentMenu = this->Menu->addMenu("&Recent") << pqSetName("Recent");
    this->loadRecentlyUsedItems();
    this->populateRecentlyUsedMenu(recentMenu);
    }

  // Add categories.
  pqInternal::CategoryInfoMap::iterator categoryIter = 
    this->Internal->Categories.begin();
  for (; categoryIter != this->Internal->Categories.end(); ++categoryIter)
    {
    QMenu* categoryMenu = this->Menu->addMenu(categoryIter.value().Label)
      << pqSetName(categoryIter.key());
    QList<QAction*> action_list;
    foreach (QString pname, categoryIter.value().Proxies)
      {
      QAction* action = this->getAction(pname);
      if (action)
        {
        // build an action list, so that we can sort it and then add to the
        // menu (BUG #8364).
        action_list.push_back(action);
        }
      }
    if (categoryIter.value().PreserveOrder == false)
      {
      // sort unless the XML overrode the sorting using the "preserve_order"
      // attribute.
      qSort(action_list.begin(), action_list.end(), ::actionTextSort);
      }
    foreach (QAction* action, action_list)
      {
      categoryMenu->addAction(action);
      }
    }

  // Add alphabetical list.
  QMenu* alphabeticalMenu = this->Menu;
  if (this->Internal->Categories.size() > 0 || this->RecentlyUsedMenuSize > 0)
    {
    alphabeticalMenu = this->Menu->addMenu("&Alphabetical")
      << pqSetName("Alphabetical");
    }

  pqInternal::ProxyInfoMap::iterator proxyIter = 
    this->Internal->Proxies.begin();

  QList<QAction*> actions;
  for (; proxyIter != this->Internal->Proxies.end(); ++proxyIter)
    {
    QAction* action = this->getAction(proxyIter.key());
    if (action)
      {
      actions.push_back(action);
      }
    }

  // Now sort all actions added in temp based on their texts.
  qSort(actions.begin(), actions.end(), ::actionTextSort);
  foreach (QAction* action, actions)
    {
    alphabeticalMenu->addAction(action);
    }

  emit this->menuPopulated();
}

//-----------------------------------------------------------------------------
QAction* pqProxyMenuManager::getAction(const QString& pname)
{
  if (pname.isEmpty())
    {
    return 0;
    }

  // Since Proxies map keeps the QAction instance, we will reuse the QAction
  // instance whenever possible.
  pqInternal::ProxyInfoMap::iterator iter = this->Internal->Proxies.find(pname);
  if (iter == this->Internal->Proxies.end())
    {
    return 0;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* prototype = pxm->GetPrototypeProxy(
    this->XMLGroup.toAscii().data(), pname.toAscii().data());
  if (prototype)
    {
    QString label = prototype->GetXMLLabel()? prototype->GetXMLLabel() : pname;
    QAction* action = iter.value().Action;
    if (!action)
      {
      action = new QAction(this);
      action << pqSetName(pname) << pqSetData(pname);
      }
    action->setText(label);
    QString icon = this->Internal->Proxies[pname].Icon;

    // Try to add some default icons if none is specified.
    if (icon.isEmpty() && prototype->IsA("vtkSMCompoundSourceProxy"))
      {
      icon = ":/pqWidgets/Icons/pqBundle32.png";
      }

    if (!icon.isEmpty())
      {
      action->setIcon(QIcon(icon));
      }

    QObject::connect(action, SIGNAL(triggered(bool)), 
      this, SLOT(triggered()), Qt::QueuedConnection);
    return action;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pqProxyMenuManager::loadRecentlyUsedItems()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = QString("recent.%1/").arg(this->ElementTagName);
  if (settings->contains(key))
    {
    QString list = settings->value(key).toString();
    this->Internal->RecentlyUsed = list.split("|", QString::SkipEmptyParts);
    }
  else
    {
    this->Internal->RecentlyUsed.clear();
    }
}

//-----------------------------------------------------------------------------
void pqProxyMenuManager::saveRecentlyUsedItems()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = QString("recent.%1/").arg(this->ElementTagName);
  settings->setValue(key, this->Internal->RecentlyUsed.join("|"));
}

//-----------------------------------------------------------------------------
void pqProxyMenuManager::updateFromXML()
{
  QDir dir(this->FilteringXMLDir);
  QStringList files = dir.entryList(QDir::Files);
  foreach(QString file, files)
    {
    if (QFileInfo(file).suffix() == "xml")
      {
      this->updateFromXML(this->FilteringXMLDir + QString("/") + file);
      }
    }
}

//-----------------------------------------------------------------------------
void pqProxyMenuManager::updateFromXML(const QString& xmlfilename)
{
  QFile xml(xmlfilename);
  if (!xml.open(QIODevice::ReadOnly))
    {
    qDebug() << "Failed to load " << xmlfilename;
    return;
    }

  QByteArray dat = xml.readAll();
  vtkSmartPointer<vtkPVXMLParser> parser = 
    vtkSmartPointer<vtkPVXMLParser>::New();
  if(!parser->Parse(dat.data()))
    {
    xml.close();
    return;
    }

  vtkPVXMLElement* root = parser->GetRootElement();

  // Iterate over Category elements and find items with tag name ElementTagName.
  // Iterate over elements with tag this->ElementTagName and add them to the
  // this->Internal->Proxies map.
  unsigned int numElems = root->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* curElem = root->GetNestedElement(cc);
    if (!curElem || !curElem->GetName())
      {
      continue;
      }

    if (strcmp(curElem->GetName(), "Category") == 0 &&
      curElem->GetAttribute("name"))
      {
      // We need to ascertain if this group is for the elements we are concerned
      // with. i.e. is there atleast one element with tag ElementTagName in this
      // category?
      if (!curElem->FindNestedElementByName(this->ElementTagName.toAscii().data()))
        {
        continue;
        }
      QString categoryName = curElem->GetAttribute("name");
      QString categoryLabel = curElem->GetAttribute("menu_label")?
        curElem->GetAttribute("menu_label") : categoryName;
      int preserve_order = 0;
      curElem->GetScalarAttribute("preserve_order", &preserve_order);

      // Valid category encountered. Update the internal datastructures.
      pqInternal::CategoryInfo& category = this->Internal->Categories[categoryName];
      category.Label = categoryLabel;
      category.PreserveOrder = (preserve_order==1);
      unsigned int numCategoryElems = curElem->GetNumberOfNestedElements();
      for (unsigned int kk=0; kk < numCategoryElems; ++kk)
        {
        vtkPVXMLElement* child = curElem->GetNestedElement(kk);
        if (child && child->GetName() && this->ElementTagName == child->GetName())
          {
          const char* name = child->GetAttribute("name");
          const char* icon = child->GetAttribute("icon");
          if (!name)
            {
            continue;
            }

          this->Internal->addProxy(name, icon);
          if (!category.Proxies.contains(name))
            {
            category.Proxies.push_back(name);
            }
          }
        }
      }
    else if (this->ElementTagName == curElem->GetName())
      {
      const char* name = curElem->GetAttribute("name");
      const char* icon = curElem->GetAttribute("icon");
      if (!name)
        {
        continue;
        }

      this->Internal->addProxy(name, icon);
      }
    }
}
