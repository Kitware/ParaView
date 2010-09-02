// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqNetCDFPanel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "pqNetCDFPanel.h"

#include "vtkSMProxy.h"

#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidget.h"

#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QMultiMap>
#include <QtDebug>

//=============================================================================
class pqNetCDFPanel::DimensionMapType : public QMultiMap<QString, QString> {};

//=============================================================================
pqNetCDFPanel::pqNetCDFPanel(pqProxy *object_proxy, QWidget *_parent)
  : Superclass(object_proxy, _parent),
    Dimensions(NULL), Variables(NULL)
{
  this->DimensionMap = new pqNetCDFPanel::DimensionMapType();

  // Find the combo box used to select the dimensions to load.  We will listen
  // to signals when this changes so that we can update the reference list of
  // variables.
  this->Dimensions = this->findChild<QComboBox*>("Dimensions");
  QLabel *dimensionsLabel = this->findChild<QLabel*>("_labelForDimensions");
  if (!this->Dimensions || !dimensionsLabel)
    {
    qWarning() << "Failed to locate Dimensions widget.";
    return;
    }

  // Underneath the dimension we want to put a widget that shows all the
  // variables that have those dimensions.  These are the variables that will be
  // loaded.  We do this by removing the dimensions widget and its label and
  // replacing it with a layout containing these and a variables list.
  int oldIndex = this->PanelLayout->indexOf(this->Dimensions);
  int row, column, rowSpan, columnSpan;
  this->PanelLayout->getItemPosition(oldIndex, &row, &column,
                                     &rowSpan, &columnSpan);
  this->PanelLayout->removeWidget(this->Dimensions);

  oldIndex = this->PanelLayout->indexOf(dimensionsLabel);
  this->PanelLayout->getItemPosition(oldIndex, &row, &column,
                                     &rowSpan, &columnSpan);
  this->PanelLayout->removeWidget(dimensionsLabel);

  this->Variables = new pqTreeWidget(this);
  this->Variables->setHeaderLabel("Variables");

  this->Variables->setToolTip("This is a list of variables that have the\n"
                              "dimensions that are selected above.  This\n"
                              "list is provided for reference only.  You\n"
                              "cannot directly edit this list.  To change\n"
                              "the variables loaded, select a different set\n"
                              "of dimensions above.");

  QGridLayout *subLayout = new QGridLayout();
  subLayout->addWidget(dimensionsLabel, 0, 0, 1, 1);
  subLayout->addWidget(this->Dimensions, 0, 1, 1, 1);
  subLayout->addWidget(this->Variables, 1, 0, 1, 2);
  subLayout->setMargin(0);
  subLayout->setSpacing(4);

  this->PanelLayout->addLayout(subLayout, row, 0, 1, -1);

  // Get the dimensions of all the variables, build a map from dimensions to
  // variables, and populate the Variables widget.
  vtkSMProxy *smproxy = object_proxy->getProxy();

  QList<QVariant> dimensions = pqSMAdaptor::getMultipleElementProperty(
                                 smproxy->GetProperty("VariableDimensionInfo"));
  QList<QVariant> variables = pqSMAdaptor::getMultipleElementProperty(
                                     smproxy->GetProperty("VariableArrayInfo"));
  if (dimensions.size() != variables.size())
    {
    qWarning() << "Sizes of Variable names and dimension arrays are different?";
    }
  else
    {
    for (int i = 0; i < dimensions.size(); i++)
      {
      QString var = variables[i].toString();
      QString dim = dimensions[i].toString();
      this->DimensionMap->insert(dim, var);
      }
    }

  if (this->Dimensions->count() > 0)
    {
    // Select the last entry in the Dimensions.  The early entries are usually
    // variables that only specify things like the dimension positions.
    this->Dimensions->setCurrentIndex(this->Dimensions->count() - 1);

    // Updated the variables listed.
    QObject::connect(this->Dimensions, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(updateVariableStatusEntries()));

    // Make sure the variables list is populated.
    this->updateVariableStatusEntries();
    }

  // Some parameters only take effect when loading spherical coordinates.
  // Disable them if the spherical coordinates option is off.
  QWidget *SphericalCoordinates
    = this->findChild<QWidget*>("SphericalCoordinates");
  QStringList dependentWidgets;
  dependentWidgets << "VerticalScale" << "_labelForVerticalScale"
                   << "VerticalBias" << "_labelForVerticalBias";
  foreach (QString dWidgetName, dependentWidgets)
    {
    QWidget *dWidget = this->findChild<QWidget*>(dWidgetName);
    QObject::connect(SphericalCoordinates, SIGNAL(toggled(bool)),
                     dWidget, SLOT(setEnabled(bool)));
    }
}

//-----------------------------------------------------------------------------
pqNetCDFPanel::~pqNetCDFPanel()
{
  delete this->DimensionMap;
}

//-----------------------------------------------------------------------------
void pqNetCDFPanel::updateVariableStatusEntries()
{
  // Remove all the entries from the VariableStatus widget.
  this->Variables->clear();

  // Add all variables with dimensions that agree with that selected.
  QString dimension = this->Dimensions->currentText();
  QList<QString> variables = this->DimensionMap->values(dimension);
  foreach(QString var, variables)
    {
    QTreeWidgetItem *item = new QTreeWidgetItem(this->Variables);
    item->setText(0, var);
    item->setFlags(Qt::NoItemFlags);
    }
}
