/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqDataSetModel.h"
#include "pqElementInspectorWidget.h"

#include <QHBoxLayout>
#include <QTreeView>
#include <QVBoxLayout>

#include <vtkUnstructuredGrid.h>

//////////////////////////////////////////////////////////////////////////////
// pqElementInspectorWidget::pqImplementation

struct pqElementInspectorWidget::pqImplementation
{
  QTreeView TreeView;
};

/////////////////////////////////////////////////////////////////////////////////
// pqElementInspectorWidget

pqElementInspectorWidget::pqElementInspectorWidget(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  this->setObjectName("ElementInspectorWidget");
  this->Implementation->TreeView.setRootIsDecorated(false);
  //this->Implementation->TreeView.setAlternatingRowColors(true);
  this->Implementation->TreeView.setModel(new pqDataSetModel(&this->Implementation->TreeView));

  QHBoxLayout* const hbox = new QHBoxLayout();
  hbox->setMargin(0);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->setMargin(0);
  vbox->addLayout(hbox);
  vbox->addWidget(&this->Implementation->TreeView);
  this->setLayout(vbox);
}

pqElementInspectorWidget::~pqElementInspectorWidget()
{
  delete this->Implementation;
}

void pqElementInspectorWidget::clearElements()
{
  qobject_cast<pqDataSetModel*>(this->Implementation->TreeView.model())->setDataSet(0);
  this->Implementation->TreeView.reset();
  this->Implementation->TreeView.update();
  emit elementsChanged(0);
}

void pqElementInspectorWidget::setElements(vtkUnstructuredGrid* Elements)
{
  qobject_cast<pqDataSetModel*>(this->Implementation->TreeView.model())->setDataSet(Elements);
  this->Implementation->TreeView.reset();
  this->Implementation->TreeView.update();
  emit elementsChanged(Elements);
}
