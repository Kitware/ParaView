/*=========================================================================

   Program: ParaView
   Module:    pqViewContextMenuManager.cxx

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

/// \file pqViewContextMenuManager.cxx
/// \date 9/19/2007

#include "pqViewContextMenuManager.h"

#include "pqView.h"
#include "pqViewContextMenuHandler.h"

#include <QMap>
#include <QString>

class pqViewContextMenuManagerInternal
{
public:
  pqViewContextMenuManagerInternal();
  ~pqViewContextMenuManagerInternal() {}

  QMap<QString, pqViewContextMenuHandler*> Handlers;
};

//----------------------------------------------------------------------------
pqViewContextMenuManagerInternal::pqViewContextMenuManagerInternal()
  : Handlers()
{
}

//----------------------------------------------------------------------------
pqViewContextMenuManager::pqViewContextMenuManager(QObject* parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqViewContextMenuManagerInternal();
}

pqViewContextMenuManager::~pqViewContextMenuManager()
{
  delete this->Internal;
}

bool pqViewContextMenuManager::registerHandler(
  const QString& viewType, pqViewContextMenuHandler* handler)
{
  if (!handler)
  {
    return false;
  }

  // Make sure the view type doesn't already have a handler.
  QMap<QString, pqViewContextMenuHandler*>::Iterator iter = this->Internal->Handlers.find(viewType);
  if (iter != this->Internal->Handlers.end())
  {
    return false;
  }

  this->Internal->Handlers.insert(viewType, handler);
  return true;
}

void pqViewContextMenuManager::unregisterHandler(pqViewContextMenuHandler* handler)
{
  if (!handler)
  {
    return;
  }

  // Find all the view types with the given handler.
  QMap<QString, pqViewContextMenuHandler*>::Iterator iter = this->Internal->Handlers.begin();
  while (iter != this->Internal->Handlers.end())
  {
    if (*iter == handler)
    {
      iter = this->Internal->Handlers.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}

bool pqViewContextMenuManager::isRegistered(pqViewContextMenuHandler* handler) const
{
  QMap<QString, pqViewContextMenuHandler*>::ConstIterator iter = this->Internal->Handlers.begin();
  for (; iter != this->Internal->Handlers.end(); ++iter)
  {
    if (*iter == handler)
    {
      return true;
    }
  }

  return false;
}

pqViewContextMenuHandler* pqViewContextMenuManager::getHandler(const QString& viewType) const
{
  QMap<QString, pqViewContextMenuHandler*>::ConstIterator iter =
    this->Internal->Handlers.find(viewType);
  if (iter != this->Internal->Handlers.end())
  {
    return *iter;
  }

  return 0;
}

void pqViewContextMenuManager::setupContextMenu(pqView* view)
{
  QMap<QString, pqViewContextMenuHandler*>::Iterator iter =
    this->Internal->Handlers.find(view->getViewType());
  if (iter != this->Internal->Handlers.end())
  {
    (*iter)->setupContextMenu(view);
  }
}

void pqViewContextMenuManager::cleanupContextMenu(pqView* view)
{
  QMap<QString, pqViewContextMenuHandler*>::Iterator iter =
    this->Internal->Handlers.find(view->getViewType());
  if (iter != this->Internal->Handlers.end())
  {
    (*iter)->cleanupContextMenu(view);
  }
}
