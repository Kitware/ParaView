/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowserContextMenu.h

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

/// \file pqPipelineBrowserContextMenu.h
/// \date 4/20/2006

#ifndef _pqPipelineBrowserContextMenu_h
#define _pqPipelineBrowserContextMenu_h


#include "pqComponentsExport.h"
#include <QObject>

class pqPipelineBrowser;
class pqPipelineBrowserContextMenuInternal;
class QAction;
class QPoint;


class PQCOMPONENTS_EXPORT pqPipelineBrowserContextMenu : public QObject
{
  Q_OBJECT

public:
  pqPipelineBrowserContextMenu(pqPipelineBrowser *browser);
  virtual ~pqPipelineBrowserContextMenu();

  enum ActionTypes
    {
    OPEN,
    ADD_SOURCE,
    ADD_FILTER,
    CREATE_CUSTOM_FILTER,
    CHANGE_INPUT,
    DELETE,
    IGNORE_TIME
    };

  // Add an action to the menu.
  void setMenuAction(ActionTypes type, QAction *action);

public slots:
  void showContextMenu(const QPoint &pos);

private:
  pqPipelineBrowserContextMenuInternal *Internal;
  pqPipelineBrowser *Browser;
};

#endif
