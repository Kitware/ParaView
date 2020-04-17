/*=========================================================================

   Program: ParaView
   Module:  pqSubsetInclusionLatticeWidget.cxx

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
#include "pqSubsetInclusionLatticeWidget.h"

#include "pqTreeView.h"
#include "pqTreeViewSelectionHelper.h"

#include <QHeaderView>
#include <QIdentityProxyModel>
#include <QTabWidget>
#include <QVBoxLayout>

namespace
{
class ProxyModel : public QIdentityProxyModel
{
public:
  ProxyModel(
    const QString& headerLabel, const QModelIndex& srcRootIndex, QObject* parentObject = nullptr)
    : QIdentityProxyModel(parentObject)
    , SourceRootIndex(srcRootIndex)
    , HeaderLabel(headerLabel)
  {
  }

  ~ProxyModel() {}

  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
  {
    if (orientation == Qt::Horizontal && section == 0)
    {
      switch (role)
      {
        case Qt::DisplayRole:
          return this->HeaderLabel;

        case Qt::TextAlignmentRole:
          return QVariant(Qt::AlignLeft | Qt::AlignVCenter);

        case Qt::CheckStateRole:
          return this->headerCheckState();

        default:
          break;
      }
    }
    return this->QIdentityProxyModel::headerData(section, orientation, role);
  }

  bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value,
    int role = Qt::EditRole) override
  {
    if (orientation == Qt::Horizontal && section == 0)
    {
      switch (role)
      {
        case Qt::CheckStateRole:
          this->setHeaderCheckState(value);
          Q_EMIT this->headerDataChanged(orientation, section, section);
          return true;

        default:
          break;
      }
    }
    return this->QIdentityProxyModel::setHeaderData(section, orientation, value, role);
  }

private:
  Q_DISABLE_COPY(ProxyModel);
  QPersistentModelIndex SourceRootIndex;
  QString HeaderLabel;

  inline QVariant headerCheckState() const
  {
    return this->SourceRootIndex.isValid()
      ? this->sourceModel()->data(this->SourceRootIndex, Qt::CheckStateRole)
      : QVariant();
  }

  inline void setHeaderCheckState(const QVariant& state)
  {
    if (this->SourceRootIndex.isValid())
    {
      this->sourceModel()->setData(this->SourceRootIndex, state, Qt::CheckStateRole);
    }
  }
};
}

class pqSubsetInclusionLatticeWidget::pqInternals
{
public:
  QPointer<QAbstractItemModel> Model;
  QPointer<QTabWidget> TabWidget;
  QList<QPointer<pqTreeView> > Widgets;
};

//-----------------------------------------------------------------------------
pqSubsetInclusionLatticeWidget::pqSubsetInclusionLatticeWidget(
  QAbstractItemModel* amodel, QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqSubsetInclusionLatticeWidget::pqInternals())
{
  pqInternals& internals = (*this->Internals);
  internals.TabWidget = new QTabWidget(this);
  internals.TabWidget->setObjectName("TabWidget");

  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->addWidget(internals.TabWidget);
  vbox->setMargin(0);

  if (amodel)
  {
    internals.Model = amodel;
    this->connect(amodel, SIGNAL(modelReset()), SLOT(modelReset()));
    /*
    this->connect(amodel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
        SLOT(rowsInserted(const QModelIndex&, int, int)));
    this->connect(amodel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
        SLOT(rowsRemoved(const QModelIndex&, int, int)));
    this->connect(amodel, SIGNAL(rowsMoved(const QModelIndex&, int, int, const QModelIndex&, int,
    int))
        SLOT(rowsMoved(const QModelIndex&, int, int, const QModelIndex&, int, int)));
    */
  }

  this->modelReset();
}

//-----------------------------------------------------------------------------
pqSubsetInclusionLatticeWidget::~pqSubsetInclusionLatticeWidget()
{
}

//-----------------------------------------------------------------------------
QAbstractItemModel* pqSubsetInclusionLatticeWidget::model() const
{
  return this->Internals->Model;
}

//-----------------------------------------------------------------------------
void pqSubsetInclusionLatticeWidget::modelReset()
{
  pqInternals& internals = (*this->Internals);

  internals.TabWidget->clear();
  for (pqTreeView* tree : internals.Widgets)
  {
    delete tree;
  }
  internals.Widgets.clear();

  QAbstractItemModel* amodel = this->model();
  if (amodel == nullptr)
  {
    return;
  }

  const int count = amodel->rowCount(QModelIndex());
  for (int cc = 0; cc < count; ++cc)
  {
    const QModelIndex idx = amodel->index(cc, 0, QModelIndex());
    const QString label = amodel->data(idx, Qt::DisplayRole).toString();

    pqTreeView* atree = new pqTreeView(this, /*use_pqHeaderView=*/true);

    ProxyModel* pmodel = new ProxyModel(label, idx, atree);
    pmodel->setSourceModel(amodel);

    atree->setObjectName(QString("Tree%1").arg(cc));
    atree->setModel(pmodel);
    atree->setRootIndex(pmodel->mapFromSource(idx));
    atree->setUniformRowHeights(true);
    atree->setRootIsDecorated(true);
    atree->expandAll();

    new pqTreeViewSelectionHelper(atree);
    internals.TabWidget->addTab(atree, label);
  }
}
