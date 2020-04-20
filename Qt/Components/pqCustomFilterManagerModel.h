/*=========================================================================

   Program: ParaView
   Module:    pqCustomFilterManagerModel.h

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

========================================================================*/

/**
* \file pqCustomFilterManagerModel.h
* \date 6/23/2006
*/

#ifndef _pqCustomFilterManagerModel_h
#define _pqCustomFilterManagerModel_h

#include "pqComponentsModule.h"
#include <QAbstractListModel>

class pqCustomFilterManagerModelInternal;
class QString;

/**
* \class pqCustomFilterManagerModel
* \brief
*   The pqCustomFilterManagerModel class stores the list of registered
*   pipeline custom filter definitions.
*
* The list is modified using the \c addCustomFilter and
* \c removeCustomFilter methods. When a new custom filter is added
* to the model a signal is emitted. This signal can be used to
* highlight the new custom filter.
*/
class PQCOMPONENTS_EXPORT pqCustomFilterManagerModel : public QAbstractListModel
{
  Q_OBJECT

public:
  pqCustomFilterManagerModel(QObject* parent = 0);
  ~pqCustomFilterManagerModel() override;

  /**
  * \name QAbstractItemModel Methods
  */
  //@{
  /**
  * \brief
  *   Gets the number of rows for a given index.
  * \param parent The parent index.
  * \return
  *   The number of rows for the given index.
  */
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
  * \brief
  *   Gets a model index for a given location.
  * \param row The row number.
  * \param column The column number.
  * \param parent The parent index.
  * \return
  *   A model index for the given location.
  */
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

  /**
  * \brief
  *   Gets the data for a given model index.
  * \param index The model index.
  * \param role The role to get data for.
  * \return
  *   The data for the given model index.
  */
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /**
  * \brief
  *   Gets the flags for a given model index.
  *
  * The flags for an item indicate if it is enabled, editable, etc.
  *
  * \param index The model index.
  * \return
  *   The flags for the given model index.
  */
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  //@}

  /**
  * \name Index Mapping Methods
  */
  //@{
  /**
  * \brief
  *   Gets the custom filter name for the given model index.
  * \param index The model index to look up.
  * \return
  *   The custom filter definition name or an empty string.
  */
  QString getCustomFilterName(const QModelIndex& index) const;

  /**
  * \brief
  *   Gets the model index for the given custom filter name.
  * \param filter The custom filter definition name to look up.
  * \return
  *   The model index for the given name.
  */
  QModelIndex getIndexFor(const QString& filter) const;
  //@}

public Q_SLOTS:
  /**
  * \brief
  *   Adds a new custom filter definition to the model.
  * \param name The name of the new custom filter definition.
  */
  void addCustomFilter(QString name);

  /**
  * \brief
  *   Removes a custom filter definition from the model.
  * \param name The name of the custom filter definition.
  */
  void removeCustomFilter(QString name);

  /**
  * Save/Load custom filters from pqSettings
  */
  void importCustomFiltersFromSettings();
  void exportCustomFiltersToSettings();

Q_SIGNALS:
  /**
  * \brief
  *   Emitted when a new custom filter definition is added to the
  *   model.
  * \param name The name of the new custom filter definition.
  */
  void customFilterAdded(const QString& name);

private:
  /**
  * Stores the custom filter list.
  */
  pqCustomFilterManagerModelInternal* Internal;
};

#endif
