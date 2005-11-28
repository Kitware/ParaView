
/// \file pqPipelineListWidget.cxx
/// \brief
///   The pqPipelineListWidget class is used to display the pipeline
///   in the form of a list.
///
/// \date 11/25/2005

#include "pqPipelineListWidget.h"

#include "pqPipelineListModel.h"

#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>


pqPipelineListWidget::pqPipelineListWidget(QWidget *parent)
  : QWidget(parent)
{
  this->ListModel = 0;
  this->TreeView = 0;

  // Create the pipeline list model.
  this->ListModel = new pqPipelineListModel(this);
  if(this->ListModel)
    this->ListModel->setObjectName("PipelineList");

  // Create a tree view to display the pipeline.
  this->TreeView = new QTreeView(this);
  if(this->TreeView)
    {
    this->TreeView->setObjectName("PipelineView");
    this->TreeView->header()->hide();
    this->TreeView->setModel(this->ListModel);

    // Make sure the tree items get expanded when a new
    // sub-item is added.
    if(this->ListModel)
      {
      connect(this->ListModel, SIGNAL(childAdded(const QModelIndex &)),
          this->TreeView, SLOT(expand(const QModelIndex &)));
      }
    }

  // Add the tree view to the layout.
  QVBoxLayout *layout = new QVBoxLayout(this);
  if(layout)
    {
    layout->setMargin(0);
    layout->addWidget(this->TreeView);
    }
}

pqPipelineListWidget::~pqPipelineListWidget()
{
}


