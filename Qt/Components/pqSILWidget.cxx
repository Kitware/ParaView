/*=========================================================================

   Program: ParaView
   Module:    pqSILWidget.cxx

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
#include "pqSILWidget.h"

#include <QAction>
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxySILModel.h"
#include "pqSILModel.h"
#include "pqSelectionManager.h"
#include "pqTreeView.h"
#include "pqTreeViewSelectionHelper.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVCompositeDataInformationIterator.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSILModel.h"
#include "vtkSMSourceProxy.h"

//-----------------------------------------------------------------------------
pqSILWidget::pqSILWidget(const QString& activeCategory, QWidget* parentObject)
  : QWidget(parentObject)
  , ActiveCategory(activeCategory)
{
  // create tab widget
  this->TabWidget = new QTabWidget(this);
  this->TabWidget->setObjectName("TabWidget");

  // create block selection buttons
  QPushButton* checkSelectedBlocksButton = new QPushButton("Check Selected Blocks", this);
  checkSelectedBlocksButton->setObjectName("CheckSelectedBlocksButton");
  this->connect(checkSelectedBlocksButton, SIGNAL(clicked()), this, SLOT(checkSelectedBlocks()));
  QPushButton* uncheckSelectedBlocksButton = new QPushButton("Uncheck Selected Blocks", this);
  uncheckSelectedBlocksButton->setObjectName("UncheckSelectedBlocksButton");
  this->connect(
    uncheckSelectedBlocksButton, SIGNAL(clicked()), this, SLOT(uncheckSelectedBlocks()));

  // setup layout
  QVBoxLayout* layout_ = new QVBoxLayout;
  layout_->setMargin(0);
  layout_->addWidget(this->TabWidget);
  QHBoxLayout* buttonLayout = new QHBoxLayout;
  buttonLayout->addWidget(checkSelectedBlocksButton);
  buttonLayout->addWidget(uncheckSelectedBlocksButton);
  layout_->addLayout(buttonLayout);
  this->setLayout(layout_);

  // setup model
  this->ActiveModel = new pqProxySILModel(activeCategory, this);
  this->SortModel = new QSortFilterProxyModel(this);
  this->SortModel->setRecursiveFilteringEnabled(true);
  this->SortModel->setSourceModel(this->ActiveModel);
}

//-----------------------------------------------------------------------------
pqSILWidget::~pqSILWidget()
{
  delete this->ActiveModel;
}

//-----------------------------------------------------------------------------
void pqSILWidget::setModel(pqSILModel* curmodel)
{
  if (this->Model)
  {
    QObject::disconnect(this->Model, 0, this, 0);
  }
  this->Model = curmodel;
  this->ActiveModel->setSourceModel(this->Model);
  if (curmodel)
  {
    QObject::connect(curmodel, SIGNAL(modelReset()), this, SLOT(onModelReset()));
  }
  this->onModelReset();
}

//-----------------------------------------------------------------------------
pqSILModel* pqSILWidget::model() const
{
  return this->Model;
}

//-----------------------------------------------------------------------------
void pqSILWidget::onModelReset()
{
  this->TabWidget->clear();
  foreach (pqTreeView* view, this->Trees)
  {
    delete view;
  }
  this->Trees.clear();

  // First add the active-tree.
  pqTreeView* activeTree = new pqTreeView(this, /*use_pqHeaderView=*/true);
  activeTree->header()->setStretchLastSection(true);
  activeTree->setUniformRowHeights(true);
  activeTree->setRootIsDecorated(false);
  activeTree->setModel(this->SortModel);
  activeTree->expandAll();
  this->TabWidget->addTab(activeTree, this->ActiveCategory);
  new pqTreeViewSelectionHelper(activeTree);
  // even for QT_VERSION < 5.10, we let filtering be enabled on the first widget
  // since in most cases this will be a flat tree where filtering works fine
  // even for older Qt versions without support for recursive filtering.

  int num_tabs = this->Model->rowCount();
  for (int cc = 0; cc < num_tabs; cc++)
  {
    if (this->Model->data(this->Model->index(cc, 0)).toString() == this->ActiveCategory)
    {
      continue;
    }

    pqTreeView* tree = new pqTreeView(this, /*use_pqHeaderView=*/true);
    tree->setUniformRowHeights(true);
    tree->header()->setStretchLastSection(true);
    tree->setRootIsDecorated(false);

    QString category = this->Model->data(this->Model->index(cc, 0)).toString();
    pqProxySILModel* proxyModel = new pqProxySILModel(category, tree);
    proxyModel->setSourceModel(this->Model);

    QSortFilterProxyModel* sortModel = new QSortFilterProxyModel(tree);
    sortModel->setRecursiveFilteringEnabled(true);
    sortModel->setSourceModel(proxyModel);
    tree->setModel(sortModel);
    tree->expandAll();

    new pqTreeViewSelectionHelper(tree);

    this->TabWidget->addTab(tree, category);
  }
}

//-----------------------------------------------------------------------------
void pqSILWidget::toggleSelectedBlocks(bool checked)
{
  pqSelectionManager* selMan =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SelectionManager"));
  if (!selMan || !selMan->getSelectedPort())
  {
    return;
  }

  pqOutputPort* port = selMan->getSelectedPort();
  vtkSMProxy* activeSelection = port->getSelectionInput();
  vtkPVDataInformation* dataInfo = port->getDataInformation();

  std::vector<vtkIdType> block_ids;
  if (activeSelection->GetProperty("Blocks"))
  {
    vtkSMPropertyHelper blocksProp(activeSelection, "Blocks");
    block_ids.resize(blocksProp.GetNumberOfElements());
    blocksProp.Get(&block_ids[0], blocksProp.GetNumberOfElements());
    std::sort(block_ids.begin(), block_ids.end());
  }
  else if (activeSelection->GetXMLName() == std::string("CompositeDataIDSelectionSource") &&
    activeSelection->GetProperty("IDs"))
  {
    vtkSMPropertyHelper idsProp(activeSelection, "IDs");

    // Elements in the block_ids are 3-tuples (block flat-index, process number, id).
    // Remove the process number and id elements and put the block ids into a set -
    // we need only the unique block ids.
    std::set<int> unique_ids;
    for (unsigned int i = 0; i < idsProp.GetNumberOfElements(); i += 3)
    {
      unique_ids.insert(idsProp.GetAsInt(i));
    }
    block_ids.resize(unique_ids.size());

    // ids will already be sorted coming out of the set.
    block_ids.insert(block_ids.begin(), unique_ids.begin(), unique_ids.end());
  }
  else
  {
    // Exit early as we don't know what blocks have been selected.
    return;
  }

  std::sort(block_ids.begin(), block_ids.end());
  auto new_end = std::unique(block_ids.begin(), block_ids.end());
  block_ids.erase(new_end, block_ids.end());

  // if check is true then we are checking only the selected blocks,
  // if check is false, then we are un-checking the selected blocks, leaving
  // the selections for the other blocks as they are.
  if (checked)
  {
    this->Model->setData(this->Model->makeIndex(0), Qt::Unchecked, Qt::CheckStateRole);
  }

  // block selection only has the block ids, now we need to convert the block
  // ids to names for the blocks (and sets) using the data information.
  vtkPVCompositeDataInformationIterator* iter = vtkPVCompositeDataInformationIterator::New();
  iter->SetDataInformation(dataInfo);
  unsigned int cur_index = 0;
  for (iter->InitTraversal();
       !iter->IsDoneWithTraversal() && cur_index < static_cast<unsigned int>(block_ids.size());
       iter->GoToNextItem())
  {
    if (static_cast<vtkIdType>(iter->GetCurrentFlatIndex()) < block_ids[cur_index])
    {
      continue;
    }
    if (static_cast<vtkIdType>(iter->GetCurrentFlatIndex()) > block_ids[cur_index])
    {
      qDebug() << "Failed to locate block's name for block id: " << block_ids[cur_index];
      cur_index++;
      continue;
    }

    vtkIdType vertexid = this->Model->findVertex(iter->GetCurrentName());
    if (vertexid != -1)
    {
      this->Model->setData(this->Model->makeIndex(vertexid), checked ? Qt::Checked : Qt::Unchecked,
        Qt::CheckStateRole);
    }
    else
    {
      // if vertexid==-1 from the SIL, then it's possible that this is a name of
      // one of the sets, since currently, sets are not part of the SIL. Until
      // the users ask for it, we will leave enabling/disabling the sets out.
    }
    cur_index++;
  }

  iter->Delete();
}
