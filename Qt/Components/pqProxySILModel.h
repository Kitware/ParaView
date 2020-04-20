/*=========================================================================

   Program: ParaView
   Module:    pqProxySILModel.h

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
#ifndef pqProxySILModel_h
#define pqProxySILModel_h

#include "pqComponentsModule.h"
#include "pqTimer.h"
#include <QAbstractProxyModel>

#include <QPixmap>

/**
* pqProxySILModel is a proxy model for pqSILModel. This makes it possible for
* tree views to show only a sub-tree in the SIL. This provides API to
* get/set status values which is useful for property linking using
* pqPropertyManager or pqPropertyLinks.
*/
class PQCOMPONENTS_EXPORT pqProxySILModel : public QAbstractProxyModel
{
  Q_OBJECT
  typedef QAbstractProxyModel Superclass;
  Q_PROPERTY(QList<QVariant> values READ values WRITE setValues)

public:
  pqProxySILModel(const QString& hierarchyName, QObject* parent = 0);
  ~pqProxySILModel() override;

  /**
  * \name QAbstractItemModel Methods
  */
  //@{
  /**
  * \brief
  *   Gets the number of rows for a given index.
  * \param theParent The parent index.
  * \return
  *   The number of rows for the given index.
  */
  int rowCount(const QModelIndex& theParent = QModelIndex()) const override
  {
    return this->sourceModel()->rowCount(this->mapToSource(theParent));
  }

  /**
  * \brief
  *   Gets the number of columns for a given index.
  * \param theParent The parent index.
  * \return
  *   The number of columns for the given index.
  */
  int columnCount(const QModelIndex& theParent = QModelIndex()) const override
  {
    return this->sourceModel()->columnCount(this->mapToSource(theParent));
  }

  /**
  * \brief
  *   Gets whether or not the given index has child items.
  * \param theParent The parent index.
  * \return
  *   True if the given index has child items.
  */
  bool hasChildren(const QModelIndex& theParent = QModelIndex()) const override
  {
    return this->sourceModel()->hasChildren(this->mapToSource(theParent));
  }

  /**
  * \brief
  *   Gets a model index for a given location.
  * \param row The row number.
  * \param column The column number.
  * \param theParent The parent index.
  * \return
  *   A model index for the given location.
  */
  QModelIndex index(
    int row, int column, const QModelIndex& theParent = QModelIndex()) const override
  {
    QModelIndex sourceIndex = this->sourceModel()->index(row, column, this->mapToSource(theParent));
    return this->mapFromSource(sourceIndex);
  }

  /**
  * \brief
  *   Gets the parent for a given index.
  * \param theIndex The model index.
  * \return
  *   A model index for the parent of the given index.
  */
  QModelIndex parent(const QModelIndex& theIndex) const override
  {
    QModelIndex sourceIndex = this->sourceModel()->parent(this->mapToSource(theIndex));
    return this->mapFromSource(sourceIndex);
  }

  /**
  * \brief
  *  Sets the role data for the item at index to value. Returns
  *  true if successful; otherwise returns false.
  */
  bool setData(const QModelIndex& theIndex, const QVariant& value, int role = Qt::EditRole) override
  {
    return this->sourceModel()->setData(this->mapToSource(theIndex), value, role);
  }
  //@}

  /**
  * Methods from QAbstractProxyModel.
  */
  QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
  QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
  void setSourceModel(QAbstractItemModel* sourceModel) override;

  /**
  * Overridden to return the same name as the hierarchy.
  *
  * Also overridden to handle Qt::CheckStateRole for pqHeaderView to support
  * toggling column checkstate from the header.
  */
  QVariant headerData(int, Qt::Orientation, int role = Qt::DisplayRole) const override;

  /**
   * overridden to handle toggling of check state.
   */
  bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value,
    int role = Qt::EditRole) override;

  /**
  * Overridden to provide a means of turning off checkboxes
  */
  QVariant data(const QModelIndex& proxyIndex, int role = Qt::DisplayRole) const override;

  /**
  * overridden to allow us to turn off checkboxes in the flags returned
  * from the model
  */
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  /**
  * Get the status values for the hierarchy.
  */
  QList<QVariant> values() const;

  /**
  * Checkboxes for each item can be disabled by setting this flag
  */
  void setNoCheckBoxes(bool val);

  /**
  * Override the display of the title in the header with this string
  */
  void setHeaderTitle(QString& title);

public Q_SLOTS:
  /**
  * Set the status values for the hierarchy.
  */
  void setValues(const QList<QVariant>&);

Q_SIGNALS:
  void valuesChanged();

protected Q_SLOTS:
  void sourceDataChanged(const QModelIndex& idx1, const QModelIndex& idx2)
  {
    QModelIndex pidx1 = this->mapFromSource(idx1);
    QModelIndex pidx2 = this->mapFromSource(idx2);
    if (!pidx1.isValid() || !pidx2.isValid())
    {
      // index is root, that mean header data may have changed as well.
      Q_EMIT this->headerDataChanged(Qt::Horizontal, 0, 0);
    }
    Q_EMIT this->dataChanged(pidx1, pidx2);
  }

  void onCheckStatusChanged();

private:
  Q_DISABLE_COPY(pqProxySILModel)

  pqTimer DelayedValuesChangedSignalTimer;
  QPixmap CheckboxPixmaps[3];
  QString HierarchyName;
  bool noCheckBoxes;
  QString HeaderTitle;
};

#endif
