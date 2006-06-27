/*=========================================================================

   Program: ParaView
   Module:    pqPipelineMenu.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqPipelineMenu.h
/// \date 6/5/2006

#ifndef _pqPipelineMenu_h
#define _pqPipelineMenu_h


#include "pqWidgetsExport.h"
#include <QObject>

class pqPipelineMenuInternal;
class pqSourceInfoGroupMap;
class pqSourceInfoIcons;
class pqSourceInfoModel;
class QAction;
class QMenu;
class QMenuBar;
class QStringList;
class vtkPVXMLElement;
class vtkSMProxy;


class PQWIDGETS_EXPORT pqPipelineMenu : public QObject
{
  Q_OBJECT

public:
  enum ActionName
    {
    InvalidAction = -1,
    AddSourceAction = 0,
    AddFilterAction,
    LastAction = AddFilterAction
    };

public:
  pqPipelineMenu(QObject *parent=0);
  virtual ~pqPipelineMenu();

  pqSourceInfoIcons *getIcons() const;

  void loadSourceInfo(vtkPVXMLElement *root);
  void loadFilterInfo(vtkPVXMLElement *root);

  pqSourceInfoModel *getFilterModel();

  void addActionsToMenuBar(QMenuBar *menubar) const;
  void addActionsToMenu(QMenu *menu) const;
  QAction *getMenuAction(ActionName name) const;

public slots:
  void addSource();
  void addFilter();

private:
  void setupConnections(pqSourceInfoModel *model, pqSourceInfoGroupMap *map);
  void getAllowedSources(pqSourceInfoModel *model, vtkSMProxy *input,
      QStringList &list);

private:
  pqPipelineMenuInternal *Internal;
  QAction **MenuList;
};

#endif
