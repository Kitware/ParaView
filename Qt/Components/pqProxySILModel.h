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
* tree views to show only a sub-tree in the SIL. This also provides API to
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
  ~pqProxySILModel();

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
  virtual int rowCount(const QModelIndex& theParent = QModelIndex()) const
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
  virtual int columnCount(const QModelIndex& theParent = QModelIndex()) const
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
  virtual bool hasChildren(const QModelIndex& theParent = QModelIndex()) const
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
  virtual QModelIndex index(int row, int column, const QModelIndex& theParent = QModelIndex()) const
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
  virtual QModelIndex parent(const QModelIndex& theIndex) const
  {
    QModelIndex sourceIndex = this->sourceModel()->parent(this->mapToSource(theIndex));
    return this->mapFromSource(sourceIndex);
  }

  /**
  * \brief
  *  Sets the role data for the item at index to value. Returns
  *  true if successful; otherwise returns false.
  */
  bool setData(const QModelIndex& theIndex, const QVariant& value, int role = Qt::EditRole)
  {
    return this->sourceModel()->setData(this->mapToSource(theIndex), value, role);
  }
  //@}

  /**
  * Methods from QAbstractProxyModel.
  */
  virtual QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;
  virtual QModelIndex mapToSource(const QModelIndex& proxyIndex) const;
  virtual void setSourceModel(QAbstractItemModel* sourceModel);

  /**
  * Overridden to return the same name as the hierarchy.
  * Also returns a DecorationRole icon which can show the check state of the
  * root node. Connect the header's sectionClicked() signal to
  * toggleRootCheckState() to support affecting the check state using the
  * header.
  */
  virtual QVariant headerData(int, Qt::Orientation, int role = Qt::DisplayRole) const;

  /**
  * Overridden to provide a means of turning off checkboxes
  */
  virtual QVariant data(const QModelIndex& proxyIndex, int role = Qt::DisplayRole) const;

  /**
  * overridden to allow us to turn off checkboxes in the flags returned
  * from the model
  */
  virtual Qt::ItemFlags flags(const QModelIndex& index) const;

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

public slots:
  /**
  * Set the status values for the hierarchy.
  */
  void setValues(const QList<QVariant>&);

  /**
  * Convenience slot to toggle the check state of the entire subtree shown by
  * this model.
  */
  void toggleRootCheckState();

signals:
  void valuesChanged();

protected slots:
  void sourceDataChanged(const QModelIndex& idx1, const QModelIndex& idx2)
  {
    QModelIndex pidx1 = this->mapFromSource(idx1);
    QModelIndex pidx2 = this->mapFromSource(idx2);
    if (!pidx1.isValid() || !pidx2.isValid())
    {
      // index is root, that mean header data may have changed as well.
      emit this->headerDataChanged(Qt::Horizontal, 0, 0);
    }
    emit this->dataChanged(pidx1, pidx2);
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
