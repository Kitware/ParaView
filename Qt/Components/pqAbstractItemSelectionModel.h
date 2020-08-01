/*=========================================================================

   Program: ParaView
   Module:  pqAbstractItemSelectionModel.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include <QAbstractItemModel>

class QTreeWidgetItem;
class vtkSMProxy;

/**
* @brief Abstract class implementing a tree model with checkable items.
* It uses QTreeWidgetItem as its item class. Reimplement the virtual methods
* to fill it with data.
*/
class pqAbstractItemSelectionModel : public QAbstractItemModel
{
  Q_OBJECT

protected:
  pqAbstractItemSelectionModel(QObject* parent_ = NULL);
  ~pqAbstractItemSelectionModel() override;

  /**
  * @{
  * QAbstractItemModel implementation
  */
  int rowCount(const QModelIndex& parent_ = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent_ = QModelIndex()) const override;

  QModelIndex index(int row, int column, const QModelIndex& parent_ = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index_) const override;
  QVariant data(const QModelIndex& index_, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index_, const QVariant& value, int role) override;
  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& index_) const override;
  /**
  * @}
  */

  /**
  * Concrete classes should implement how the model is to be populated.
  */
  virtual void populateModel(void* dataObject) = 0;

  /**
  * Initialize the root item which holds the header tags.
  */
  virtual void initializeRootItem() = 0;

  /**
  * Helper for a more comprehensive validation of indices.
  */
  bool isIndexValid(const QModelIndex& index_) const;

  /**
  *
  */

  QTreeWidgetItem* RootItem;
};
