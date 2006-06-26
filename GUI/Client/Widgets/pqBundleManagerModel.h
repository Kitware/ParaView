/*=========================================================================

   Program: ParaView
   Module:    pqBundleManagerModel.h

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

/// \file pqBundleManagerModel.h
/// \date 6/23/2006

#ifndef _pqBundleManagerModel_h
#define _pqBundleManagerModel_h


#include "pqWidgetsExport.h"
#include <QAbstractListModel>

class pqBundleManagerModelInternal;
class QString;


/// \class pqBundleManagerModel
/// \brief
///   The pqBundleManagerModel class stores the list of registered
///   pipeline bundle definitions.
///
/// The list is modified using the \c addBundle and \c removeBundle
/// methods. When a new bundle is added to the model a signal is
/// emitted. This signal can be used to highlight the new bundle.
class PQWIDGETS_EXPORT pqBundleManagerModel : public QAbstractListModel
{
  Q_OBJECT

public:
  pqBundleManagerModel(QObject *parent=0);
  virtual ~pqBundleManagerModel();

  /// \name QAbstractItemModel Methods
  //@{
  /// \brief
  ///   Gets the number of rows for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of rows for the given index.
  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;

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

  /// \name Index Mapping Methods
  //@{
  /// \brief
  ///   Gets the bundle name for the given model index.
  /// \param index The model index to look up.
  /// \return
  ///   The bundle definition name or an empty string.
  QString getBundleName(const QModelIndex &index) const;

  /// \brief
  ///   Gets the model index for the given bundle name.
  /// \param bundle The bundle definition name to look up.
  /// \return
  ///   The model index for the given name.
  QModelIndex getIndexFor(const QString &bundle) const;
  //@}

public slots:
  /// \brief
  ///   Adds a new pipeline bundle definition to the model.
  /// \param name The name of the new pipeline bundle definition.
  void addBundle(QString name);

  /// \brief
  ///   Removes a pipeline bundle definition from the model.
  /// \param name The name of the pipeline bundle definition.
  void removeBundle(QString name);

signals:
  /// \brief
  ///   Emitted when a new bundle definition is added to the model.
  /// \param name The name of the new pipeline bundle definition.
  void bundleAdded(const QString &name);

private:
  pqBundleManagerModelInternal *Internal; ///< Stores the bundle list.
};

#endif
