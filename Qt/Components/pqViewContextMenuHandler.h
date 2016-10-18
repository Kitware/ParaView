/*=========================================================================

   Program: ParaView
   Module:    pqViewContextMenuHandler.h

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
* \file pqViewContextMenuHandler.h
* \date 9/19/2007
*/

#ifndef _pqViewContextMenuHandler_h
#define _pqViewContextMenuHandler_h

#include "pqComponentsModule.h"
#include <QObject>

class pqView;

/**
* \class pqViewContextMenuHandler
* \brief
*   The pqViewContextMenuHandler class is used to setup and cleanup
*   the context menu for a view of a given type.
*/
class PQCOMPONENTS_EXPORT pqViewContextMenuHandler : public QObject
{
  Q_OBJECT

public:
  /**
  * \brief
  *   Constructs a view context menu handler.
  * \param parent The parent object.
  */
  pqViewContextMenuHandler(QObject* parent = 0);
  virtual ~pqViewContextMenuHandler() {}

  /**
  * \brief
  *   Sets up the context menu for the given view.
  *
  * The pqViewContextMenuManager maps the view type to the correct
  * handler and calls this method to set up the context menu.
  *
  * \param view The view to set up.
  */
  virtual void setupContextMenu(pqView* view) = 0;

  /**
  * \brief
  *   Cleans up the context menu for the given view.
  * \param view The view to clean up.
  */
  virtual void cleanupContextMenu(pqView* view) = 0;
};

#endif
