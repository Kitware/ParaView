/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowserContextMenu.cxx

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

/// \file pqPipelineBrowserContextMenu.cxx
/// \date 4/20/2006

#include "pqPipelineBrowserContextMenu.h"

#include "pqFlatTreeView.h"
#include "pqPipelineBrowser.h"

#include <QItemSelectionModel>
#include <QMenu>
#include <QPointer>


//-----------------------------------------------------------------------------
class pqPipelineBrowserContextMenuInternal
{
public:
  pqPipelineBrowserContextMenuInternal();
  ~pqPipelineBrowserContextMenuInternal() {}

  QPointer<QAction> OpenFile;
  QPointer<QAction> AddSource;
  QPointer<QAction> AddFilter;
  QPointer<QAction> CustomFilter;
  QPointer<QAction> ChangeInput;
  QPointer<QAction> Delete;
  QPointer<QAction> IgnoreTime;
};

//-----------------------------------------------------------------------------
pqPipelineBrowserContextMenuInternal::pqPipelineBrowserContextMenuInternal()
  : OpenFile(), AddSource(), AddFilter(), CustomFilter(), ChangeInput(),
    Delete()
{
}

//-----------------------------------------------------------------------------
pqPipelineBrowserContextMenu::pqPipelineBrowserContextMenu(
    pqPipelineBrowser *browser)
  : QObject(browser)
{
  this->Internal = new pqPipelineBrowserContextMenuInternal();
  this->Browser = browser;

  this->setObjectName("ContextMenu");

  // Listen for the custom context menu signal.
  if(this->Browser)
    {
    QObject::connect(this->Browser->getTreeView(),
        SIGNAL(customContextMenuRequested(const QPoint &)),
        this, SLOT(showContextMenu(const QPoint &)));
    }
}

//-----------------------------------------------------------------------------
pqPipelineBrowserContextMenu::~pqPipelineBrowserContextMenu()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqPipelineBrowserContextMenu::setMenuAction(
  ActionTypes type, QAction *action)
{
  if (!action)
    {
    return;
    }

  switch (type)
    {
  case OPEN:
    this->Internal->OpenFile = action;
    break;

  case ADD_SOURCE:
    this->Internal->AddSource = action;
    break;

  case ADD_FILTER:
    this->Internal->AddFilter = action;
    break;

  case CREATE_CUSTOM_FILTER:
    this->Internal->CustomFilter = action;
    break;

  case CHANGE_INPUT:
    this->Internal->ChangeInput = action;
    break;

  case DELETE:
    this->Internal->Delete = action;
    break;

  case IGNORE_TIME:
    this->Internal->IgnoreTime = action;
    }
}

//-----------------------------------------------------------------------------
void pqPipelineBrowserContextMenu::showContextMenu(const QPoint &pos)
{
  if(!this->Browser)
    {
    return;
    }

  QMenu menu;
  menu.setObjectName("PipelineObjectMenu");

  // Get the selected indexes from the browser.
  bool addSep = false;
  QModelIndexList indexes =
      this->Browser->getSelectionModel()->selectedIndexes();

  // Add the 'open' item to the menu.
  if(!this->Internal->OpenFile.isNull())
    {
    menu.addAction(this->Internal->OpenFile);
    addSep = true;
    }

  // Add the 'add source' item to the menu.
  if(!this->Internal->AddSource.isNull())
    {
    menu.addAction(this->Internal->AddSource);
    addSep = true;
    }

  if(indexes.size() > 0)
    {
    // Add the 'add filter' item to the menu.
    if(!this->Internal->AddFilter.isNull())
      {
      menu.addAction(this->Internal->AddFilter);
      addSep = true;
      }

    // Add the 'create custom filter' item to the menu.
    if(!this->Internal->CustomFilter.isNull())
      {
      menu.addAction(this->Internal->CustomFilter);
      addSep = true;
      }

    if(addSep)
      {
      menu.addSeparator();
      addSep = false;
      }

    // Add the 'change input' item to the menu.
    if(!this->Internal->ChangeInput.isNull() &&
        this->Internal->ChangeInput->isEnabled())
      {
      menu.addAction(this->Internal->ChangeInput);
      }

    // Add "ignore time" item.
    if (!this->Internal->IgnoreTime.isNull() &&
      this->Internal->IgnoreTime->isEnabled())
      {
      menu.addAction(this->Internal->IgnoreTime);
      }

    // Add the 'delete' item to the menu.
    if(!this->Internal->Delete.isNull())
      {
      menu.addAction(this->Internal->Delete);
      }
    

    }

  if(menu.actions().size() > 0)
    {
    menu.exec(this->Browser->getTreeView()->mapToGlobal(pos));
    }
}

