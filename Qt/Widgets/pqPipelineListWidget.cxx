
/// \file pqPipelineListWidget.cxx
/// \brief
///   The pqPipelineListWidget class is used to display the pipeline
///   in the form of a list.
///
/// \date 11/25/2005

#include "pqPipelineListWidget.h"

#include "pqPipelineData.h"
#include "pqPipelineListModel.h"
#include "QVTKWidget.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"

#include <QHeaderView>
#include <QItemSelectionModel>
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

    // Listen to the selection change signals.
    QItemSelectionModel *selection = this->TreeView->selectionModel();
    if(selection)
      {
      connect(selection,
          SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
          this, SLOT(changeCurrent(const QModelIndex &, const QModelIndex &)));
      }

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

vtkSMProxy *pqPipelineListWidget::getSelectedProxy() const
{
  vtkSMProxy *proxy = 0;
  if(this->ListModel && this->TreeView)
    {
    // Get the current item from the model.
    QModelIndex current = this->TreeView->selectionModel()->currentIndex();
    proxy = this->ListModel->getProxyFor(current);
    }

  return proxy;
}

vtkSMProxy *pqPipelineListWidget::getNextProxy() const
{
  vtkSMProxy *proxy = 0;
  if(this->ListModel && this->TreeView)
    {
    // Get the current item from the model. Make sure the current item
    // is a proxy object.
    QModelIndex current = this->TreeView->selectionModel()->currentIndex();
    if(this->ListModel->getProxyFor(current))
      {
      current = this->ListModel->sibling(current.row() + 1, 0, current);
      proxy = this->ListModel->getProxyFor(current);
      }
    }

  return proxy;
}

QVTKWidget *pqPipelineListWidget::getCurrentWindow() const
{
  QVTKWidget *qvtk = 0;
  if(this->ListModel && this->TreeView)
    {
    // First, check to see if there is a selection.
    QModelIndex current = this->TreeView->selectionModel()->currentIndex();
    if(current.isValid())
      {
      // See if the selected item is a window.
      QWidget *widget = this->ListModel->getWidgetFor(current);
      if(widget)
        qvtk = qobject_cast<QVTKWidget *>(widget);
      else
        {
        // If the selected item is not a window, get it from the
        // proxy's parent window.
        vtkSMProxy *proxy = this->ListModel->getProxyFor(current);
        pqPipelineData *pipeline = pqPipelineData::instance();
        if(proxy && pipeline)
          qvtk = pipeline->getWindowFor(proxy);
        }
      }
    }

  return qvtk;
}

void pqPipelineListWidget::selectProxy(vtkSMProxy *proxy)
{
  if(this->ListModel && this->TreeView)
    {
    QModelIndex index = this->ListModel->getIndexFor(proxy);
    this->TreeView->selectionModel()->setCurrentIndex(index,
        QItemSelectionModel::Select | QItemSelectionModel::Current |
        QItemSelectionModel::Clear);
    }
}

void pqPipelineListWidget::selectWindow(QVTKWidget *window)
{
  if(this->ListModel && this->TreeView)
    {
    QModelIndex index = this->ListModel->getIndexFor(window);
    this->TreeView->selectionModel()->setCurrentIndex(index,
        QItemSelectionModel::Select | QItemSelectionModel::Current |
        QItemSelectionModel::Clear);
    }
}

void pqPipelineListWidget::changeCurrent(const QModelIndex &current,
    const QModelIndex &previous)
{
  if(this->ListModel)
    {
    // Get the current item from the model.
    vtkSMProxy *proxy = this->ListModel->getProxyFor(current);
    emit this->proxySelected(vtkSMSourceProxy::SafeDownCast(proxy));
    }
}


