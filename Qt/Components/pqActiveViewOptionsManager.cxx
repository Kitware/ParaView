/*=========================================================================

   Program: ParaView
   Module:    pqActiveViewOptionsManager.cxx

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

/// \file pqActiveViewOptionsManager.cxx
/// \date 7/27/2007

#include "pqActiveViewOptionsManager.h"

#include "pqActiveViewOptions.h"
#include "pqCoreUtilities.h"
#include "pqView.h"

#include <QMap>
#include <QString>
#include <QtDebug>
#include <QWidget>

//-----------------------------------------------------------------------------
class pqActiveViewOptionsManagerInternal
{
public:
  pqActiveViewOptionsManagerInternal();
  ~pqActiveViewOptionsManagerInternal() {}

  QMap<QString, pqActiveViewOptions *> Handlers;
  pqActiveViewOptions *Current;
  pqView *ActiveView;
  bool IgnoreClose;
};


//----------------------------------------------------------------------------
pqActiveViewOptionsManagerInternal::pqActiveViewOptionsManagerInternal()
  : Handlers()
{
  this->Current = 0;
  this->ActiveView = 0;
  this->IgnoreClose = false;
}


//----------------------------------------------------------------------------
pqActiveViewOptionsManager::pqActiveViewOptionsManager(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqActiveViewOptionsManagerInternal();
}

//-----------------------------------------------------------------------------
pqActiveViewOptionsManager::~pqActiveViewOptionsManager()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
bool pqActiveViewOptionsManager::registerOptions(const QString &viewType,
    pqActiveViewOptions *options)
{
  if(!options)
    {
    return false;
    }

  // Make sure the view type doesn't already have a handler.
  QMap<QString, pqActiveViewOptions *>::Iterator iter =
      this->Internal->Handlers.find(viewType);
  if(iter != this->Internal->Handlers.end())
    {
    return false;
    }

  this->Internal->Handlers.insert(viewType, options);
  this->connect(options, SIGNAL(optionsClosed(pqActiveViewOptions *)),
    this, SLOT(removeCurrent(pqActiveViewOptions *)));

  return true;
}

//-----------------------------------------------------------------------------
void pqActiveViewOptionsManager::unregisterOptions(
    pqActiveViewOptions *options)
{
  if(!options)
    {
    return;
    }

  // Find all the view types with the given handler.
  QMap<QString, pqActiveViewOptions *>::Iterator iter =
      this->Internal->Handlers.begin();
  while(iter != this->Internal->Handlers.end())
    {
    if(*iter == options)
      {
      iter = this->Internal->Handlers.erase(iter);
      }
    else
      {
      ++iter;
      }
    }

  this->disconnect(options, 0, this, 0);
  if(options == this->Internal->Current)
    {
    // Close the options dialog.
    this->Internal->Current->closeOptions();
    this->Internal->Current = 0;
    }
}

//-----------------------------------------------------------------------------
bool pqActiveViewOptionsManager::isRegistered(
    pqActiveViewOptions *options) const
{
  QMap<QString, pqActiveViewOptions *>::ConstIterator iter =
      this->Internal->Handlers.begin();
  for( ; iter != this->Internal->Handlers.end(); ++iter)
    {
    if(*iter == options)
      {
      return true;
      }
    }

  return false;
}

//-----------------------------------------------------------------------------
pqActiveViewOptions *pqActiveViewOptionsManager::getOptions(
    const QString &viewType) const
{
  QMap<QString, pqActiveViewOptions *>::ConstIterator iter =
      this->Internal->Handlers.find(viewType);
  if(iter != this->Internal->Handlers.end())
    {
    return *iter;
    }

  return 0;
}

//-----------------------------------------------------------------------------
void pqActiveViewOptionsManager::setActiveView(pqView *view)
{
  this->Internal->ActiveView = view;
  if(this->Internal->Current)
    {
    pqActiveViewOptions *options = this->getCurrent();
    if(this->Internal->Current == options)
      {
      // Change the view for the current options dialog.
      this->Internal->Current->changeView(view);
      }
    else
      {
      // Clean up the current options dialog.
      this->Internal->IgnoreClose = true;
      this->Internal->Current->closeOptions();
      this->Internal->Current->changeView(0);
      this->Internal->IgnoreClose = false;
      this->Internal->Current = options;
      if(this->Internal->Current)
        {
        // Open the options dialog for the new active view.
        this->Internal->Current->showOptions(this->Internal->ActiveView,
            QString(),
            pqCoreUtilities::mainWidget());
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqActiveViewOptionsManager::showOptions()
{
  this->showOptions(QString());
}

//-----------------------------------------------------------------------------
void pqActiveViewOptionsManager::showOptions(const QString &page)
{
  if(this->Internal->Current || !this->Internal->ActiveView)
    {
    return;
    }

  this->Internal->Current = this->getCurrent();
  if(this->Internal->Current)
    {
    this->Internal->Current->showOptions(this->Internal->ActiveView, page,
      pqCoreUtilities::mainWidget());
    }
  else
    {
    qWarning() << "An options dialog is not available for the active view.";
    }
}

//-----------------------------------------------------------------------------
void pqActiveViewOptionsManager::removeCurrent(pqActiveViewOptions *options)
{
  if(!this->Internal->IgnoreClose && options == this->Internal->Current)
    {
    this->Internal->Current = 0;
    }
}

//-----------------------------------------------------------------------------
pqActiveViewOptions *pqActiveViewOptionsManager::getCurrent() const
{
  pqActiveViewOptions *options = 0;
  if(this->Internal->ActiveView)
    {
    QMap<QString, pqActiveViewOptions *>::Iterator iter =
        this->Internal->Handlers.find(
        this->Internal->ActiveView->getViewType());
    if(iter != this->Internal->Handlers.end())
      {
      options = *iter;
      }
    }

  return options;
}

//-----------------------------------------------------------------------------
bool pqActiveViewOptionsManager::canShowOptions(pqView* view) const
{
  pqView* oldCur = this->Internal->ActiveView;
  this->Internal->ActiveView = view;
  pqActiveViewOptions* options = this->getCurrent();
  this->Internal->ActiveView = oldCur;
  return (options != NULL);
}
