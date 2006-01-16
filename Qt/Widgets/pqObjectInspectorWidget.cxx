
/// \file pqObjectInspectorWidget.cxx
/// \brief
///   The pqObjectInspectorWidget class is used to display the properties
///   of an object in an editable list.
///
/// \date 11/25/2005

#include "pqObjectInspectorWidget.h"

#include "pqObjectInspector.h"
#include "pqObjectInspectorDelegate.h"

#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>


pqObjectInspectorWidget::pqObjectInspectorWidget(QWidget *p)
  : QWidget(p)
{
  this->Inspector = 0;
  this->Delegate = 0;
  this->TreeView = 0;

  // Create the object inspector model.
  this->Inspector = new pqObjectInspector(this);
  if(this->Inspector)
    this->Inspector->setObjectName("Inspector");

  // Create the delegate to work with the model.
  this->Delegate = new pqObjectInspectorDelegate(this);

  // Create the tree view to display the model.
  this->TreeView = new QTreeView(this);
  if(this->TreeView)
    {
    this->TreeView->setObjectName("InspectorView");
    this->TreeView->setAlternatingRowColors(true);
    this->TreeView->header()->hide();
    this->TreeView->setModel(this->Inspector);
    if(this->Delegate)
      this->TreeView->setItemDelegate(this->Delegate);
    }

  // Add the tree view to the layout.
  QVBoxLayout *boxLayout = new QVBoxLayout(this);
  if(boxLayout)
    {
    boxLayout->setMargin(0);
    boxLayout->addWidget(this->TreeView);
    }
}

pqObjectInspectorWidget::~pqObjectInspectorWidget()
{
  if(this->TreeView)
    this->TreeView->setModel(0);
}

void pqObjectInspectorWidget::setProxy(vtkSMProxy *proxy)
{
  if(this->Inspector)
    {
    // remember expanded items
    int count = this->Inspector->rowCount();
    QList<bool> expanded;
    for(int i=0; i<count; i++)
      {
      if(this->TreeView->isExpanded(this->Inspector->index(i,0)))
        expanded.append(true);
      else
        expanded.append(false);
      }

    this->Inspector->setProxy(proxy);
    
    // if less rows than before (was empty), make all expanded
    int newcount = this->Inspector->rowCount();
    for(; count < newcount; count++)
      expanded.append(true);

    for(int i=0; i<newcount; i++)
      {
      this->TreeView->setExpanded(this->Inspector->index(i,0), expanded[i]);
      }
    }
}

