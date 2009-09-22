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

#include <QHeaderView>

#include "pqTreeView.h"
#include "pqProxySILModel.h"
#include "pqSILModel.h"

//-----------------------------------------------------------------------------
pqSILWidget::pqSILWidget(const QString& activeCategory, QWidget* parentObject)
  : Superclass(parentObject), ActiveCategory(activeCategory)
{
  
  this->ActiveModel = new pqProxySILModel(activeCategory, this);
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
  this->clear();
  foreach (pqTreeView* view, this->Trees)
    {
    delete view;
    }
  this->Trees.clear();

  // First add the active-tree.
  pqTreeView* activeTree = new pqTreeView(this);
  activeTree->header()->setStretchLastSection(true);
  activeTree->setRootIsDecorated(false);
  activeTree->header()->setClickable(true);
  QObject::connect(activeTree->header(), SIGNAL(sectionClicked(int)),
    this->ActiveModel, SLOT(toggleRootCheckState()), Qt::QueuedConnection);
  activeTree->setModel(this->ActiveModel);
  activeTree->expandAll();
  this->addTab(activeTree, this->ActiveCategory);

  int num_tabs = this->Model->rowCount();
  for (int cc=0; cc < num_tabs; cc++)
    {
    if (this->Model->data(this->Model->index(cc, 0)).toString() ==
      this->ActiveCategory)
      {
      continue;
      }

    pqTreeView* tree = new pqTreeView(this);
    tree->header()->setStretchLastSection(true);
    tree->setRootIsDecorated(false);

    pqProxySILModel* proxyModel = new pqProxySILModel(
      this->Model->data(this->Model->index(cc, 0)).toString(), tree);
    proxyModel->setSourceModel(this->Model);

    tree->header()->setClickable(true);
    QObject::connect(tree->header(), SIGNAL(sectionClicked(int)),
      proxyModel, SLOT(toggleRootCheckState()), Qt::QueuedConnection);
    tree->setModel(proxyModel);
    tree->expandAll();

    this->addTab(tree, proxyModel->headerData(cc, Qt::Horizontal).toString());
    }
}
