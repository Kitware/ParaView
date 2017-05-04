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

#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
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
  pqTreeView* activeTree = new pqTreeView(this);

  // pqTreeView create a pqCheckableHeaderView which we don't care for since the
  // pqSILModel handles header-state on its own. We just need to set the default
  // header.
  activeTree->setHeader(new QHeaderView(Qt::Horizontal, activeTree));
  activeTree->header()->setStretchLastSection(true);
  activeTree->setRootIsDecorated(false);
#if QT_VERSION >= 0x050000
  activeTree->header()->setSectionsClickable(true);
#else
  activeTree->header()->setClickable(true);
#endif

  QObject::connect(activeTree->header(), SIGNAL(sectionClicked(int)), this->ActiveModel,
    SLOT(toggleRootCheckState()), Qt::QueuedConnection);
  activeTree->setModel(this->SortModel);
  activeTree->expandAll();
  this->TabWidget->addTab(activeTree, this->ActiveCategory);
  new pqTreeViewSelectionHelper(activeTree);

  int num_tabs = this->Model->rowCount();
  for (int cc = 0; cc < num_tabs; cc++)
  {
    if (this->Model->data(this->Model->index(cc, 0)).toString() == this->ActiveCategory)
    {
      continue;
    }

    pqTreeView* tree = new pqTreeView(this);
    // pqTreeView create a pqCheckableHeaderView which we don't care for since the
    // pqSILModel handles header-state on its own. We just need to set the default
    // header.
    tree->setHeader(new QHeaderView(Qt::Horizontal, tree));
    tree->header()->setStretchLastSection(true);
    tree->setRootIsDecorated(false);

    pqProxySILModel* proxyModel =
      new pqProxySILModel(this->Model->data(this->Model->index(cc, 0)).toString(), tree);
    proxyModel->setSourceModel(this->Model);

#if QT_VERSION >= 0x050000
    tree->header()->setSectionsClickable(true);
#else
    tree->header()->setClickable(true);
#endif
    QObject::connect(tree->header(), SIGNAL(sectionClicked(int)), proxyModel,
      SLOT(toggleRootCheckState()), Qt::QueuedConnection);
    tree->setModel(proxyModel);
    tree->expandAll();
    new pqTreeViewSelectionHelper(tree);

    this->TabWidget->addTab(tree, proxyModel->headerData(cc, Qt::Horizontal).toString());
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

  vtkSMPropertyHelper blocksProp(activeSelection, "Blocks");
  std::vector<vtkIdType> block_ids;
  block_ids.resize(blocksProp.GetNumberOfElements());
  blocksProp.Get(&block_ids[0], blocksProp.GetNumberOfElements());
  std::sort(block_ids.begin(), block_ids.end());

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
