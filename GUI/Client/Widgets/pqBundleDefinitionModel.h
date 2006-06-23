/*=========================================================================

   Program: ParaView
   Module:    pqBundleDefinitionModel.h

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

/// \file pqBundleDefinitionModel.h
/// \date 6/19/2006

#ifndef _pqBundleDefinitionModel_h
#define _pqBundleDefinitionModel_h


#include "pqWidgetsExport.h"
#include <QAbstractItemModel>

class pqBundleDefinitionModelItem;
class pqPipelineSource;
class pqServerManagerSelection;
class QPixmap;


/// \class pqBundleDefinitionModel
/// \brief
///   The pqBundleDefinitionModel class stores the sources that define
///   a compound proxy in a hierarchical format.
///
/// The hierarchical format is similar to the pqPipelineModel. The
/// bundle definition model contains only sources. It does not include
/// any server objects, since the bundle must be defined on one server.
class PQWIDGETS_EXPORT pqBundleDefinitionModel : public QAbstractItemModel
{
public:
  enum ItemType
    {
    Invalid = -1,
    Source = 0,
    Filter,
    Bundle,
    Link,
    LastType = Link
    };

public:
  pqBundleDefinitionModel(QObject *parent=0);
  virtual ~pqBundleDefinitionModel();

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

  /// \brief
  ///   Sets the contents of the bundle definition model.
  ///
  /// The \c items are added to the bundle definition model in a
  /// hierarchy similar to the pipeline model. Only the items in the
  /// list are added to the hierarchy.
  ///
  /// \param items The list of selected model items.
  void setContents(const pqServerManagerSelection *items);

  /// \brief
  ///   Gets the next index in the model's tree hierarchy.
  /// \param index The current model index.
  /// \return
  ///   A model index for the next item in the hierarchy.
  QModelIndex getNextIndex(const QModelIndex &index) const;

  /// \brief
  ///   Gets the source associated with an index.
  /// \param index The model index to look up.
  /// \return
  ///   A pointer to the source object or null if there is none.
  pqPipelineSource *getSourceFor(const QModelIndex &index) const;

private:
  /// \brief
  ///   Gets the model object associated with the index.
  /// \param index The model index to convert.
  /// \return
  ///   A pointer to the model object or null if there is none.
  pqBundleDefinitionModelItem *getModelItemFor(const QModelIndex &index) const;

  /// \brief
  ///   Gets the next item in the tree.
  /// \param item The current model item.
  /// \return
  ///   A pointer to the next model item or null if the end is reached.
  pqBundleDefinitionModelItem *getNextItem(
      pqBundleDefinitionModelItem *item) const;

private:
  pqBundleDefinitionModelItem *Root; ///< The root of the model tree.
  QPixmap *PixmapList;               ///< Stores the item icons.
};

#endif
