/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowser.h

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

=========================================================================*/

/// \file pqPipelineBrowser.h
/// \date 4/20/2006

#ifndef _pqPipelineBrowser_h
#define _pqPipelineBrowser_h


#include "pqWidgetsExport.h"
#include <QWidget>
#include <QPointer>

class pqFlatTreeView;
class pqPipelineModel;
class pqServerManagerModelItem;
class pqPipelineSource;
class pqServer;
class pqRenderModule;
class QItemSelectionModel;
class QModelIndex;

// This is the pipeline browser widget. It creates the pqPipelineModel
// and the pqFlatTreeView. pqPipelineModel observes events from the
// pqServerManagerModel do keep the pipeline view in sync with the 
// the server manager. It provides slot (select()) to change the currently
// selected item, it also fires a signal selectionChanged() when the selection
// changes.
class PQWIDGETS_EXPORT pqPipelineBrowser : public QWidget
{
  Q_OBJECT

public:
  pqPipelineBrowser(QWidget *parent=0);
  virtual ~pqPipelineBrowser();

  virtual bool eventFilter(QObject *object, QEvent *e);

  pqPipelineModel *getListModel() const {return this->ListModel;}
  pqFlatTreeView *getTreeView() const {return this->TreeView;}

  QItemSelectionModel *getSelectionModel() const;

  /// returns the currently select object, may be a 
  /// server/source/filter.
  pqServerManagerModelItem* getCurrentSelection() const;

  /// returns the server for the currently selected branch.
  /// This is a convienience method.
  pqServer *getCurrentServer() const;
  
  /// get the render module this pipeline browser works with
  pqRenderModule *getRenderModule();

public slots:
  // Call this to select the particular item.
  void select(pqServerManagerModelItem* item);
  void select(pqPipelineSource* src);
  void select(pqServer* server);

  void deleteSelected();

  /// set the current render module for the pipeline browser
  void setRenderModule(pqRenderModule* rm);

signals:
  // Fired when the selection is changed. Argument is the newly selected
  // item.
  void selectionChanged(pqServerManagerModelItem* selectedItem);
  
  /// Fired when the render module changes
  void renderModuleChanged(pqRenderModule*);

private slots:
  void changeCurrent(const QModelIndex &current, const QModelIndex &previous);
  void handleIndexClicked(const QModelIndex &index);

private:
  pqPipelineModel *ListModel;
  pqFlatTreeView *TreeView;
  QPointer<pqRenderModule> RenderModule;
};

#endif
