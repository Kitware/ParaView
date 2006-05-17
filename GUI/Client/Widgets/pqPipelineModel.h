/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineModel.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/// \file pqPipelineModel.h
/// \date 4/14/2006

#ifndef _pqPipelineModel_h
#define _pqPipelineModel_h


#include "pqWidgetsExport.h"
#include <QAbstractItemModel>

class pqMultiView;
class pqPipelineFilter;
class pqPipelineLink;
class pqPipelineModelInternal;
class pqPipelineModelItem;
class pqPipelineObject;
class pqPipelineServer;
class pqPipelineSource;
class pqServer;
class QPixmap;
class QString;
class vtkPVXMLElement;
class vtkSMDisplayProxy;
class vtkSMProxy;
class vtkSMRenderModuleProxy;
class pqPipelineModelDataItem;

/// This class is the model for the PipelineLine browser tree view.
/// pqServerManagerModel models the vtkSMProxyManager for the GUI. The 
/// vtkSMProxyManager maintains all proxies and hence it is difficult 
/// to detect/trasvers pipelines etc etc. The pqServerManagerModel
/// provides a simplified view of the Server Manager. This class
/// takes that simplified "model" and transforms it into hierachical
/// tables which can be represented by the Tree View.
class PQWIDGETS_EXPORT pqPipelineModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  enum ItemType {
    Invalid = -1,
    Server = 0,
    Source,
    Filter,
    Bundle,
    Link
  };

public:
  pqPipelineModel(QObject *parent=0);
  virtual ~pqPipelineModel();

  /// \name QAbstractItemModel Methods
  //@{
  /// \brief
  ///   Gets the number of rows for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of rows for the given index.
  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the number of columns for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of columns for the given index.
  virtual int columnCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets whether or not the given index has child items.
  /// \param parent The parent index.
  /// \return
  ///   True if the given index has child items.
  virtual bool hasChildren(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets a model index for a given location.
  /// \param row The row number.
  /// \param column The column number.
  /// \param parent The parent index.
  /// \return
  ///   A model index for the given location.
  virtual QModelIndex index(int row, int column,
      const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the parent for a given index.
  /// \param index The model index.
  /// \return
  ///   A model index for the parent of the given index.
  virtual QModelIndex parent(const QModelIndex &index) const;

  /// \brief
  ///   Gets the data for a given model index.
  /// \param index The model index.
  /// \param role The role to get data for.
  /// \return
  ///   The data for the given model index.
  virtual QVariant data(const QModelIndex &index,
      int role=Qt::DisplayRole) const;

  /// \brief
  ///   Gets the flags for a given model index.
  ///
  /// The flags for an item indicate if it is enabled, editable, etc.
  ///
  /// \param index The model index.
  /// \return
  ///   The flags for the given model index.
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  //@}

  /// \name Object Mapping
  //@{

  /// Given the index, get the pqPipelineModelItem it represents.
  /// NULL is returned for root or invalid index.
  pqPipelineModelItem* getItem(const QModelIndex& ) const;


  QModelIndex getIndexFor(pqPipelineModelItem *item) const;
  //@}

public slots:
  /// Called when a new server connection is detected. Adds the connection to the
  /// list.
  void addServer(pqServer *server);

  /// Called when a server connection is closed. Removes the server from the list.
  void removeServer(pqServer *server);

  // Called when a new source/filter/bundle is registered.
  void addSource(pqPipelineSource* source);

  // Called when a new source/filter/bundle is unregistred.
  void removeSource(pqPipelineSource* source);

  // Called when new pipeline connection (between two pipeline objects)
  // is made.
  void addConnection(pqPipelineSource *source, pqPipelineSource *sink);

  // Called when new pipeline connection (between two pipeline objects)
  // is broken.
  void removeConnection(pqPipelineSource *source, pqPipelineSource *sink);

signals:
  void firstChildAdded(const QModelIndex &index);

public:
  void saveState(vtkPVXMLElement *root, pqMultiView *multiView=0);

private slots:
  void serverDataChanged();

private:

  // Add an item as a child under the parent at the given index.
  // Note that this method does not actually change the underlying
  // pqServerManagerModel, it merely signals that such an addition
  // has taken place.
  void addChild(pqPipelineModelDataItem* parent, 
    pqPipelineModelDataItem* child);

  // Remove a child item from under the parent.
  // Note that this method does not actually change the underlying
  // pqServerManagerModel, it merely signals that such an addition
  // has taken place.
  void removeChildFromParent(pqPipelineModelDataItem* child);

  // Returns the pqPipelineModelDataItem for the given pqPipelineModelItem.
  pqPipelineModelDataItem* getDataItem(pqPipelineModelItem* item,
    pqPipelineModelDataItem* subtreeRoot) const;

  QModelIndex getIndex(pqPipelineModelDataItem* item) const;
private:
  pqPipelineModelInternal *Internal; ///< Stores the pipeline representation.
  QPixmap *PixmapList;               ///< Stores the item icons.
  bool IgnorePipeline;               ///< Used to ignore pipeline signals.
};

#endif
