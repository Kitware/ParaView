/*=========================================================================

   Program: ParaView
   Module:    pqPipelineMenu.h

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

/// \file pqPipelineMenu.h
/// \date 6/5/2006

#ifndef _pqPipelineMenu_h
#define _pqPipelineMenu_h


#include "pqComponentsExport.h"
#include <QObject>
#include <QAbstractItemModel>

class pqPipelineModel;
class QAction;
class QItemSelectionModel;
class QModelIndex;


/// \class pqPipelineMenu
/// \brief
///   The pqPipelineMenu class is used to update the enabled state of
///   the pipeline menu.
class PQCOMPONENTS_EXPORT pqPipelineMenu : public QObject
{
  Q_OBJECT

public:
  enum ActionName
    {
    InvalidAction = -1,
    AddSourceAction = 0,
    AddFilterAction,
    ChangeInputAction,
    IgnoreTimeAction,
    DeleteAction,
    LastAction = DeleteAction
    };

public:
  pqPipelineMenu(QObject *parent=0);
  virtual ~pqPipelineMenu();

  void setModels(pqPipelineModel *model, QItemSelectionModel *selection);

  void setMenuAction(ActionName name, QAction *action);
  QAction *getMenuAction(ActionName name) const;

  bool isActionEnabled(ActionName name) const;

public slots:
  void updateActions();

private slots:
  void handleDeletion();
  void handleConnectionChange(const QModelIndex &parent);

private:
  /// Returns true if it's possible to delete the selected items.
  bool canDeleteIndexes(const QModelIndexList& indexes);

private:
  pqPipelineModel *Model;
  QItemSelectionModel *Selection;
  QAction **MenuList;
};

#endif
