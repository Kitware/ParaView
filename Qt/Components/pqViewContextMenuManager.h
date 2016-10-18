/*=========================================================================

   Program: ParaView
   Module:    pqViewContextMenuManager.h

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

/**
* \file pqViewContextMenuManager.h
* \date 9/19/2007
*/

#ifndef _pqViewContextMenuManager_h
#define _pqViewContextMenuManager_h

#include "pqComponentsModule.h"
#include <QObject>

class pqView;
class pqViewContextMenuHandler;
class pqViewContextMenuManagerInternal;
class QString;

/**
* \class pqViewContextMenuManager
* \brief
*   The pqViewContextMenuManager class manages the setup and cleanup
*   of view context menus.
*/
class PQCOMPONENTS_EXPORT pqViewContextMenuManager : public QObject
{
  Q_OBJECT

public:
  /**
  * \brief
  *   Creates a view context menu manager.
  * \param parent The parent object.
  */
  pqViewContextMenuManager(QObject* parent = 0);
  virtual ~pqViewContextMenuManager();

  /**
  * \brief
  *   Registers a context menu handler with a view type.
  * \param viewType The name of the view type.
  * \param handler The context menu handler.
  * \return
  *   True if the registration was successful.
  */
  bool registerHandler(const QString& viewType, pqViewContextMenuHandler* handler);

  /**
  * \brief
  *   Removes the context menu from the name mapping.
  * \param handler The context menu handler.
  */
  void unregisterHandler(pqViewContextMenuHandler* handler);

  /**
  * \brief
  *   Gets whether or not the context menu handler is registered.
  * \return
  *   True if the context menu handler is associated with a view type.
  */
  bool isRegistered(pqViewContextMenuHandler* handler) const;

  /**
  * \brief
  *   Gets the context menu handler for the specified view type.
  * \param viewType The name of the view type.
  * \return
  *   A pointer to the context menu handler.
  */
  pqViewContextMenuHandler* getHandler(const QString& viewType) const;

public slots:
  /**
  * \brief
  *   Sets up the context menu for the given view.
  *
  * The context menu is only set up for the view if there is a
  * registered handler for the view type.
  *
  * \param view The view to initialize.
  */
  void setupContextMenu(pqView* view);

  /**
  * \brief
  *   Cleans up the context menu for the given view.
  * \param view The view to clean up.
  */
  void cleanupContextMenu(pqView* view);

private:
  /**
  * Stores the registered context menu handlers.
  */
  pqViewContextMenuManagerInternal* Internal;
};

#endif
