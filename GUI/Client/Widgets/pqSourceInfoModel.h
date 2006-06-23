/*=========================================================================

   Program: ParaView
   Module:    pqSourceInfoModel.h

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

/// \file pqSourceInfoModel.h
/// \date 5/26/2006

#ifndef _pqSourceInfoModel_h
#define _pqSourceInfoModel_h


#include "pqWidgetsExport.h"
#include <QAbstractItemModel>

#include "pqSourceInfoIcons.h" // Needed for enum

class pqSourceInfoModelItem;
class QString;
class QStringList;


class PQWIDGETS_EXPORT pqSourceInfoModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  pqSourceInfoModel(const QStringList &sources, QObject *parent=0);
  virtual ~pqSourceInfoModel();

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

  bool isSource(const QModelIndex &index) const;

  bool isSource(const QString &name) const;

  void setIcons(pqSourceInfoIcons *icons,
      pqSourceInfoIcons::DefaultPixmap type);

public slots:
  /// \name Modification Methods
  //@{
  void clearGroups();

  void addGroup(const QString &group);
  void removeGroup(const QString &group);

  void addSource(const QString &name, const QString &group);
  void removeSource(const QString &name, const QString &group);
  //@}

private slots:
  void updatePixmap(const QString &name);

private:
  QModelIndex getIndexFor(pqSourceInfoModelItem *item) const;
  pqSourceInfoModelItem *getItemFor(const QModelIndex &index) const;
  pqSourceInfoModelItem *getGroupItemFor(const QString &group) const;

  pqSourceInfoModelItem *getChildItem(pqSourceInfoModelItem *item,
      const QString &name) const;
  bool isNameInItem(const QString &name, pqSourceInfoModelItem *item) const;

  void addChildItem(pqSourceInfoModelItem *item);
  void removeChildItem(pqSourceInfoModelItem *item);

  pqSourceInfoModelItem *getNextItem(pqSourceInfoModelItem *item) const;

private:
  pqSourceInfoModelItem *Root;
  pqSourceInfoIcons *Icons;
  pqSourceInfoIcons::DefaultPixmap Pixmap;
};

#endif
