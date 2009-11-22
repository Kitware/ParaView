/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowser.h

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

/// \file pqPipelineBrowser.h
/// \date 4/20/2006

#ifndef _pqPipelineBrowser_h
#define _pqPipelineBrowser_h


#include "pqComponentsExport.h"
#include "vtkSetGet.h"
#include <QWidget>
#include <QModelIndex> // Needed for typedef

class pqFlatTreeView;
class pqPipelineBrowserInternal;
class pqPipelineModel;
class pqPipelineSource;
class pqServer;
class pqServerManagerModelItem;
class pqView;
//class pqSourceHistoryModel;
//class pqSourceInfoGroupMap;
//class pqSourceInfoIcons;
//class pqSourceInfoModel;
class QItemSelectionModel;
class QStringList;
class vtkPVXMLElement;

// This is the pipeline browser widget. It creates the pqPipelineModel
// and the pqFlatTreeView. pqPipelineModel observes events from the
// pqServerManagerModel do keep the pipeline view in sync with the 
// the server manager. It provides slot (select()) to change the currently
// selected item, it also fires a signal selectionChanged() when the selection
// changes.
// @deprecated Replaced by pqPipelineBrowserWidget
class PQCOMPONENTS_EXPORT pqPipelineBrowser : public QWidget
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a pipeline browser instance.
  /// \param parent The parent widget.
  VTK_LEGACY(pqPipelineBrowser(QWidget *parent=0));
  virtual ~pqPipelineBrowser();

  /// \brief
  ///   Used to monitor the key press events in the tree view.
  /// \param object The object receiving the event.
  /// \param e The event information.
  /// \return
  ///   True if the event should not be sent to the object.
  virtual bool eventFilter(QObject *object, QEvent *e);

  pqPipelineModel *getModel() const {return this->Model;}
  pqFlatTreeView *getTreeView() const {return this->TreeView;}

#if 0
  pqSourceInfoIcons *getIcons() const {return this->Icons;}
#endif

  /// \name Session Continuity Methods
  //@{
#if 0
  void loadFilterInfo(vtkPVXMLElement *root);
#endif

  /// \name Selection Helper Methods
  //@{
  /// \brief
  ///   Gets the selection model from the tree view.
  /// \return
  ///   A pointer to the selection model.
  QItemSelectionModel *getSelectionModel() const;
  //@}
  
  /// get the view module this pipeline browser works with
  pqView *getView() const;

public slots:
  /// \name Model Modification Methods
  //@{
#if 0
  void addSource();
  void addFilter();
#endif
  void changeInput();
  void deleteSelected();
  //@}

  /// \brief
  ///   Sets the current render module.
  /// \param rm The current render module.
  void setView(pqView* rm);

signals:
  /// Fired when the browser begins performing an undoable change.
  void beginUndo(const QString& label);

  /// Fired when the browser is finished with the undoable change.
  void endUndo();

private slots:
  void handleIndexClicked(const QModelIndex &index);
  void handleSingleClickItem(const QModelIndex &index);
  
  /// Called when the user changes the name of a source.
  void onRename(const QModelIndex& index, const QString& name);

private:
#if 0
  pqSourceInfoModel *getFilterModel();
  void setupConnections(pqSourceInfoModel *model, pqSourceInfoGroupMap *map);
  void getAllowedSources(pqSourceInfoModel *model,
      const QModelIndexList &indexes, QStringList &list);
#endif

private:
  pqPipelineBrowserInternal *Internal; ///< Stores the class data.
  pqPipelineModel *Model;              ///< Stores the pipeline model.
  pqFlatTreeView *TreeView;            ///< Stores the tree view.
  //pqSourceInfoIcons *Icons;            ///< Stores the icons.
  //pqSourceInfoGroupMap *FilterGroups;  ///< Stores the filter grouping.
  //pqSourceHistoryModel *FilterHistory; ///< Stores the recent filters.
};

#endif
