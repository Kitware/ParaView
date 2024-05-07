// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDataInformationWidget.h"

// Qt includes.
#include <QHeaderView>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVBoxLayout>

// ParaView includes.
#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataInformationModel.h"
#include "pqNonEditableStyledItemDelegate.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqSectionVisibilityContextMenu.h"
#include "pqSelectionAdaptor.h"
#include "pqServerManagerModel.h"
#include "pqSetName.h"
#include "vtkSMOutputPort.h"

namespace
{
class pqDataInformationModelSelectionAdaptor : public pqSelectionAdaptor
{
public:
  pqDataInformationModelSelectionAdaptor(QItemSelectionModel* diModel)
    : pqSelectionAdaptor(diModel)
  {
  }

protected:
  /// subclasses can override this method to provide model specific selection
  /// overrides such as QItemSelection::Rows or QItemSelection::Columns etc.
  QItemSelectionModel::SelectionFlag qtSelectionFlags() const override
  {
    return QItemSelectionModel::Rows;
  }

  /// Maps a pqServerManagerModelItem to an index in the QAbstractItemModel.
  QModelIndex mapFromItem(pqServerManagerModelItem* item) const override
  {
    const pqDataInformationModel* pM =
      qobject_cast<const pqDataInformationModel*>(this->getQModel());

    pqOutputPort* outputPort = qobject_cast<pqOutputPort*>(item);
    if (outputPort)
    {
      return pM->getIndexFor(outputPort);
    }
    pqPipelineSource* src = qobject_cast<pqPipelineSource*>(item);
    return pM->getIndexFor(src ? src->getOutputPort(0) : nullptr);
  }

  /// Maps a QModelIndex to a pqServerManagerModelItem.
  pqServerManagerModelItem* mapToItem(const QModelIndex& index) const override
  {
    const pqDataInformationModel* pM =
      qobject_cast<const pqDataInformationModel*>(this->getQModel());
    return pM->getItemFor(index);
  }
};
}
//-----------------------------------------------------------------------------
pqDataInformationWidget::pqDataInformationWidget(QWidget* _parent /*=0*/)
  : QWidget(_parent)
{
  this->Model = new pqDataInformationModel(this);
  this->View = new QTableView(this);
  this->View->setItemDelegate(new pqNonEditableStyledItemDelegate(this));

  // We provide the sorting proxy model
  QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel(this);
  proxyModel->setSourceModel(this->Model);
  this->View->setModel(proxyModel);

  this->View->verticalHeader()->hide();
  this->View->installEventFilter(this);
  this->View->horizontalHeader()->setSectionsMovable(true);
  this->View->horizontalHeader()->setHighlightSections(false);
  this->View->horizontalHeader()->setStretchLastSection(true);
  // this->View->horizontalHeader()->setSortIndicatorShown(true);
  this->View->setSelectionBehavior(QAbstractItemView::SelectRows);

  QVBoxLayout* _layout = new QVBoxLayout(this);
  if (_layout)
  {
    _layout->setContentsMargins(0, 0, 0, 0);
    _layout->addWidget(this->View);
  }

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smModel, SIGNAL(sourceAdded(pqPipelineSource*)), this->Model,
    SLOT(addSource(pqPipelineSource*)));
  QObject::connect(smModel, SIGNAL(sourceRemoved(pqPipelineSource*)), this->Model,
    SLOT(removeSource(pqPipelineSource*)));
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this->Model,
    SLOT(setActiveView(pqView*)));
  this->Model->setActiveView(pqActiveObjects::instance().activeView());

  // Clicking on the header should sort the column.
  QObject::connect(this->View->horizontalHeader(), &QHeaderView::sectionClicked, this->View,
    [=](int col) { this->View->sortByColumn(col, Qt::AscendingOrder); });

  // Set the context menu policy for the header.
  this->View->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

  QObject::connect(this->View->horizontalHeader(),
    SIGNAL(customContextMenuRequested(const QPoint&)), this,
    SLOT(showHeaderContextMenu(const QPoint&)));

  this->View->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(this->View, SIGNAL(customContextMenuRequested(const QPoint&)), this,
    SLOT(showBodyContextMenu(const QPoint&)));

  new pqDataInformationModelSelectionAdaptor(this->View->selectionModel());
}

//-----------------------------------------------------------------------------
pqDataInformationWidget::~pqDataInformationWidget()
{
  delete this->View;
  delete this->Model;
}

//-----------------------------------------------------------------------------
bool pqDataInformationWidget::eventFilter(QObject* object, QEvent* evt)
{
  return QWidget::eventFilter(object, evt);
}

//-----------------------------------------------------------------------------
void pqDataInformationWidget::showHeaderContextMenu(const QPoint& _pos)
{
  QHeaderView* header = this->View->horizontalHeader();

  pqSectionVisibilityContextMenu menu;
  menu.setObjectName("DataInformationHeaderContextMenu");
  menu.setHeaderView(header);
  menu.exec(this->View->mapToGlobal(_pos));
}

//-----------------------------------------------------------------------------
void pqDataInformationWidget::showBodyContextMenu(const QPoint& _pos)
{
  QMenu menu;
  menu.setObjectName("DataInformationBodyContextMenu");
  QAction* action = menu.addAction(tr("Column Titles")) << pqSetName("ColumnTitles");
  action->setCheckable(true);
  action->setChecked(this->View->horizontalHeader()->isVisible());
  if (menu.exec(this->View->mapToGlobal(_pos)) == action)
  {
    this->View->horizontalHeader()->setVisible(action->isChecked());
  }
}
