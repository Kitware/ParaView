/*=========================================================================

   Program: ParaView
   Module:    pqSourceHistoryModel.h

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

/// \file pqSourceHistoryModel.h
/// \date 5/26/2006

#ifndef _pqSourceHistoryModel_h
#define _pqSourceHistoryModel_h


#include "pqComponentsExport.h"
#include <QAbstractListModel>

#include "pqSourceInfoIcons.h" // Needed for enum

class pqSourceHistoryModelInternal;
class QStringList;


/// \class pqSourceHistoryModel
/// \brief
///   The pqSourceHistoryModel class is used to store the list of
///   recent sources.
///
/// The pqSourceHistoryModel can be used for recent sources or
/// filters. The underlying data is the same for each type. The only
/// difference is the default icon type used. The default icon type
/// can be configured using the \c setIcons method.
class PQCOMPONENTS_EXPORT pqSourceHistoryModel : public QAbstractListModel
{
  Q_OBJECT

public:
  pqSourceHistoryModel(QObject *parent=0);
  virtual ~pqSourceHistoryModel();

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
  ///   Gets the source name for a given index.
  /// \param index The index to look up.
  /// \return
  ///   The source name for the given index or an empty string if the
  ///   index is not valid.
  QString getSourceName(const QModelIndex &index) const;

  /// \brief
  ///   Gets the index for the given source name.
  /// \param source The source name to look up.
  /// \return
  ///   The model index for the given source name.
  QModelIndex getIndexFor(const QString &source) const;
  //@}

  /// \name History Methods
  //@{
  /// \brief
  ///   Gets the history limit.
  /// \return
  ///   The history limit.
  int getHistoryLimit() const {return this->Limit;}

  /// \brief
  ///   Sets the history limit.
  /// \param limit The new history limit.
  void setHistoryLimit(int limit);

  /// \brief
  ///   Gets the list of sources in the history list.
  /// \param list Used to return the list of sources.
  void getHistoryList(QStringList &list) const;

  /// \brief
  ///   Sets the list of sources in the history list.
  /// \param list The list of sources.
  void setHistoryList(const QStringList &list);

  /// \brief
  ///   Adds the source to the history list.
  ///
  /// If the history list excedes the limit when adding the source,
  /// the oldest source in the list will be removed. If the source
  /// is already in the list, it will be moved to the most recent
  /// position.
  ///
  /// \param source The source name to add to the history list.
  void addRecentSource(const QString &source);
  //@}

  /// \brief
  ///   Initializes the icon database.
  /// \param icons The icon database.
  /// \param type The default icon type to display.
  void setIcons(pqSourceInfoIcons *icons,
      pqSourceInfoIcons::DefaultPixmap type);

private slots:
  /// \brief
  ///   Updates the pixmap for the given source name.
  /// \param name The name of the source whose icon changed.
  void updatePixmap(const QString &name);

private:
  pqSourceHistoryModelInternal *Internal;  ///< Stores the history list.
  pqSourceInfoIcons *Icons;                ///< A pointer to the icons.
  pqSourceInfoIcons::DefaultPixmap Pixmap; ///< The default icon type.
  int Limit;                               ///< Stores the history limit.
};

#endif
