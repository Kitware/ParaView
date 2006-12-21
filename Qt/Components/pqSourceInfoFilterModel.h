/*=========================================================================

   Program: ParaView
   Module:    pqSourceInfoFilterModel.h

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

/// \file pqSourceInfoFilterModel.h
/// \date 6/26/2006

#ifndef _pqSourceInfoFilterModel_h
#define _pqSourceInfoFilterModel_h


#include "pqComponentsExport.h"
#include <QAbstractProxyModel>

class pqSourceInfoFilterModelInternal;
class pqSourceInfoFilterModelItem;
class pqSourceInfoModel;
class QStringList;


/// \class pqSourceInfoFilterModel
/// \brief
///   The pqSourceInfoFilterModel class is used to display only the
///   allowed sources for a model.
///
/// It is designed to filter a pqSourceHistoryModel or a
/// pqSourceInfoModel. The filtering functionality is needed by the
/// "Add Filter..." dialog to only display filters that can be
/// connected to the selected input.
class PQCOMPONENTS_EXPORT pqSourceInfoFilterModel : public QAbstractProxyModel
{
  Q_OBJECT

public:
  pqSourceInfoFilterModel(QObject *parent=0);
  virtual ~pqSourceInfoFilterModel();

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

  /// \name QAbstractProxyModel Methods
  //@{
  /// \brief
  ///   Sets the source model for the proxy model.
  /// \param source The source model.
  virtual void setSourceModel(QAbstractItemModel *source);

  /// \brief
  ///   Gets the proxy model index for a given source model index.
  /// \param sourceIndex The source model index to map.
  virtual QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

  /// \brief
  ///   Gets the source model index for a given proxy model index.
  /// \param proxyIndex The proxy model index to map.
  virtual QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
  //@}

  /// \name Model Filter Methods
  //@{
  /// \brief
  ///   Sets the list of allowed names.
  ///
  /// The proxy model will load or reload the data from the source
  /// model based on the list of allowed names. If the proxy model is
  /// filtering a pqSourceInfoModel, it will pass through the group
  /// indexes and only filter the source indexes.
  ///
  /// \param allowed The list of allowed source names.
  void setAllowedNames(const QStringList &allowed);
  //@}

private slots:
  /// \brief
  ///   Loads new source model rows in the proxy model.
  /// \param sourceIndex The parent source model index.
  /// \param start The first row added.
  /// \param end The last row added.
  void addModelRows(const QModelIndex &sourceIndex, int start, int end);

  /// \brief
  ///   Removes source model rows from the proxy model.
  ///
  /// This method is called when the source model begins removing
  /// rows. It send the corresponding signal to the view and then
  /// removes the proxy model items from the data structure. The
  /// items being removed are not deleted until the source model
  /// signals that it has finished removing the rows.
  ///
  /// \param sourceIndex The parent source model index.
  /// \param start The first row to be removed.
  /// \param end The last row to be removed.
  /// \sa pqSourceInfoFilterModel::finishRemovingRows(const QModelIndex &, int, int)
  void startRemovingRows(const QModelIndex &sourceIndex, int start, int end);

  /// \brief
  ///   Removes source model rows from the proxy model.
  ///
  /// This method is called when the source model finishes removing
  /// rows. The corresponding signal is sent to the view and then the
  /// proxy model items are deleted.
  ///
  /// \param sourceIndex The parent source model index.
  /// \param start The first row to be removed.
  /// \param end The last row to be removed.
  /// \sa pqSourceInfoFilterModel::startRemovingRows(const QModelIndex &, int, int)
  void finishRemovingRows(const QModelIndex &sourceIndex, int start, int end);

  /// Reloads the source model data.
  void handleSourceReset();

private:
  /// \brief
  ///   Gets the proxy model item for a given proxy model index.
  /// \param proxyIndex The proxy model index.
  pqSourceInfoFilterModelItem *getModelItem(
      const QModelIndex &proxyIndex) const;

  /// \brief
  ///   Gets the proxy model item for a given source model index.
  /// \param sourceIndex The source model index to look up.
  pqSourceInfoFilterModelItem *getModelItemFromSource(
      const QModelIndex &sourceIndex) const;

  /// Clears the proxy model data.
  void clearData();

  /// Loads all the data from the source model.
  void loadData();

  /// \brief
  ///   Loads the source model data for a given index.
  ///
  /// This method is called recursively to load all the source model
  /// data. The source data is filtered out according to the list of
  /// allowed names.
  ///
  /// \param source A pointer to the source model for convenience.
  /// \param sourceIndex The source index to load data from.
  /// \param item The proxy model item for the source index.
  void loadData(QAbstractItemModel *source, const QModelIndex &sourceIndex,
      pqSourceInfoFilterModelItem *item);

private:
  pqSourceInfoFilterModelInternal *Internal; ///< Stores the model mapping.
  pqSourceInfoFilterModelItem *Root;         ///< Root of the proxy tree.
  pqSourceInfoModel *SourceInfo;             ///< Used in filtering.
};

#endif
