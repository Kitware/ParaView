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
  ItemType getTypeFor(const QModelIndex &index) const;

  vtkSMProxy *getProxyFor(const QModelIndex &index) const;
  pqPipelineObject *getObjectFor(const QModelIndex &index) const;
  pqPipelineSource *getSourceFor(const QModelIndex &index) const;
  pqPipelineSource *getSourceFor(vtkSMProxy *proxy) const;
  pqPipelineServer *getServerFor(const QModelIndex &index) const;
  pqPipelineServer *getServerFor(vtkSMProxy *proxy) const;
  pqPipelineServer *getServerFor(pqServer *server) const;

  QModelIndex getIndexFor(vtkSMProxy *proxy) const;
  QModelIndex getIndexFor(pqPipelineObject *object) const;
  QModelIndex getIndexFor(pqPipelineServer *server) const;
  //@}

  /// \name Server List Methods
  //@{
  int getServerCount() const;
  pqPipelineServer *getServer(int index) const;
  int getServerIndexFor(pqPipelineServer *server) const;
  //@}

public slots:
  /// \name Pipeline Management
  //@{
  void clearPipelines();

  void addServer(pqServer *server);
  void removeServer(pqServer *server);
  void removeServer(pqPipelineServer *server);

  void addWindow(QWidget *window, pqServer *server);
  void removeWindow(QWidget *window);

  void addSource(vtkSMProxy *source, const QString &name, pqServer *server);
  void addFilter(vtkSMProxy *filter, const QString &name, pqServer *server);
  void addBundle(vtkSMProxy *bundle, const QString &name, pqServer *server);

  /// \brief
  ///   Removes the proxy from the pipeline model.
  /// \param proxy The proxy to remove.
  /// \sa
  ///   pqPipelineModel::removeObject(pqPipelineSource *)
  void removeObject(vtkSMProxy *proxy);

  /// \brief
  ///   Removes the source object from the pipeline model.
  ///
  /// This method does not reconnect the surrounding proxies. When
  /// the object is removed, the broken connections will cause the
  /// view to be rearranged. If you want to reconnect the input(s)
  /// to the output(s), use the \c extractObject method instead.
  ///
  /// \param source The pipeline source object to remove.
  /// \sa
  ///   pqPipelineModel::extractObject(pqPipelineFilter *),
  ///   pqPipelineModel::removeBranch(pqPipelineSource *)
  void removeObject(pqPipelineSource *source);

  /// \brief
  ///   Removes the filter object from the pipeline model.
  ///
  /// This method can be used to remove a proxy from the pipeline and
  /// reconnect the surrounding proxies. Reconnecting the surrounding
  /// proxies reduces the amount of view changes the user sees. An
  /// object can't be extracted if it doesn't have any inputs or it
  /// doesn't have any outputs. The object can't be extracted if it
  /// has multiple inputs and multiple outputs. In case the object
  /// can't be extracted, this method automatically calls the
  /// \c removeObject method.
  ///
  /// \param filter The pipeline filter object to remove.
  /// \sa
  ///   pqPipelineModel::removeObject(pqPipelineSource *),
  ///   pqPipelineModel::removeBranch(pqPipelineSource *)
  void extractObject(pqPipelineFilter *filter);

  /// \brief
  ///   Removes the source and its output from the pipeline model.
  ///
  /// This method removes the specified object as well as the branch
  /// connected to this output. The output branch removed does not
  /// include objects with multiple inputs. The link to those objects
  /// is removed, but not the subtree associated with those objects.
  ///
  /// \param source The pipeline source object to remove.
  /// \sa
  ///   pqPipelineModel::removeObject(pqPipelineSource *),
  ///   pqPipelineModel::extractObject(pqPipelineFilter *)
  void removeBranch(pqPipelineSource *source);

  void addConnection(vtkSMProxy *source, vtkSMProxy *sink);
  void addConnection(pqPipelineSource *source, pqPipelineFilter *sink);
  void removeConnection(vtkSMProxy *source, vtkSMProxy *sink);
  void removeConnection(pqPipelineSource *source, pqPipelineFilter *sink);

  void addDisplay(vtkSMDisplayProxy *display, const QString &name,
      vtkSMProxy *proxy, vtkSMRenderModuleProxy *module);
  void removeDisplay(vtkSMDisplayProxy *display, vtkSMProxy *proxy);
  //@}

signals:
  void firstChildAdded(const QModelIndex &index);

public:
  void saveState(vtkPVXMLElement *root, pqMultiView *multiView=0);

private:
  int getItemRow(pqPipelineModelItem *item) const;
  pqPipelineModelItem *getItemParent(pqPipelineModelItem *item) const;
  void addItemAsSource(pqPipelineSource *source, pqPipelineServer *server);
  void removeLink(pqPipelineLink *link);

private:
  pqPipelineModelInternal *Internal; ///< Stores the pipeline representation.
  QPixmap *PixmapList;               ///< Stores the item icons.
  bool IgnorePipeline;               ///< Used to ignore pipeline signals.
};

#endif
