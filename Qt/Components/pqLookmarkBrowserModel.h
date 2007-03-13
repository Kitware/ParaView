/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkBrowserModel.h

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


#ifndef _pqLookmarkBrowserModel_h
#define _pqLookmarkBrowserModel_h


#include "pqComponentsExport.h"
#include <QAbstractListModel>

class pqLookmarkBrowserModelInternal;
class QString;
class QImage;
class pqLookmarkModel;
class pqLookmarkManagerModel;

/// \class pqLookmarkBrowserModel
/// \brief
///   The pqLookmarkBrowserModel class stores the list of lookmark definitions.
/// 
/// The list is modified using the \c addLookmark and
/// \c removeLookmark methods. When a new lookmark is added
/// to the model a signal is emitted. This signal can be used to
/// highlight the new lookmark.
///
/// It listens to signals from pqLookmarkManagerModel to update its list of lookmarks since lookmarks can be added, removed, modified from other views in the application.
/// 
/// A lookmark in the list can be "loaded" (i.e. have its stored server manager state loaded in vtkSMProxyManager). 
///
/// It is stored as a QString in the application's pqSetttings under the key "LookmarkBrowserState".
///
/// Still to do: Convert to a hierarchical model

class PQCOMPONENTS_EXPORT pqLookmarkBrowserModel : public QAbstractListModel
{
  Q_OBJECT

public:
  //pqLookmarkBrowserModel(QObject *parent=0);

  /// \brief
  ///   Creates a lookmark browser model from a lookmark manager model.
  /// \param other Used to build a lookmark browser model.
  /// \param parent The parent object.
  pqLookmarkBrowserModel(const pqLookmarkManagerModel *other, QObject *parent=0);

  virtual ~pqLookmarkBrowserModel();

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

  /// \brief
  ///   Gets the model index for the given lookmark name.
  /// \param filter The lookmark definition name to look up.
  /// \return
  ///   The model index for the given name.
  QModelIndex getIndexFor(const QString &name) const;
  QString getNameFor(const QModelIndex &idx) const;
  //@}

  pqLookmarkModel *getLookmarkAtIndex(const QModelIndex &idx);

public slots:
  /// \brief
  ///   Adds a new lookmark definition to the model.
  /// \param name The name of the new lookmark definition.
  /// \param image The icon of the new lookmark definition.
  /// \param state The server manager state of the new lookmark definition.
  void addLookmark(pqLookmarkModel *lmkModel);

  /// \brief
  ///   Removes a lookmark definition from the model.
  /// \param name The name of the lookmark definition.
  /// \param index The index at which the lookmark is stored.
  /// \param lmk A pointer to the lookmark object to be removed.
  /// \param selection The list of indices to remove from list.
  void removeLookmark(QString name);
  void removeLookmark(const QModelIndex &index);
  //void removeLookmark(pqLookmarkModel *lmk);
  void removeLookmarks(QModelIndexList &selection);

  /// \brief
  ///   This gets called when a lookmark has been modified somewhere else in the application and we need to update our data.
  /// \param lmk The lookmark that's been modified.
  void onLookmarkModified(pqLookmarkModel *lmk);

  /// \brief
  ///   Takes a QModelIndexList and converts it to a list of lookmarks, emitting a signal telling pqLookmarkManagerModel to do the exporting.
  /// \param selection The indices of the lookmarks to export.
  /// \param files The files to export to.
  void exportLookmarks(const QModelIndexList &selection, const QStringList &files);

signals:
  /// \brief
  ///   Emitted when a new lookmark definition is added to the
  ///   model.
  /// \param name The name of the new lookmark definition.
  void lookmarkAdded(const QString &name);

  /// \brief
  ///   Emitted when a lookmark has been removed from the model.
  /// \param name The name of the new lookmark model.
  void lookmarkRemoved(const QString &name);

  void importLookmarks(const QStringList &files);
  void exportLookmarks(const QList<pqLookmarkModel*> &list, const QStringList &files);

private:
  /// Stores the lookmark list.
  pqLookmarkBrowserModelInternal *Internal;
};

#endif
