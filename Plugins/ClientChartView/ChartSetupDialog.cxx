/*=========================================================================

   Program: ParaView
   Module:    ChartSetupDialog.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "ChartSetupDialog.h"
#include "ui_ChartSetupDialog.h"

#include <pqApplicationCore.h>
#include <pqRepresentation.h>
#include <pqImageUtil.h>
#include <pqPropertyHelper.h>
#include <pqSettings.h>

#include <vtkAbstractArray.h>
#include <vtkDataSetAttributes.h>
#include <vtkExtractVOI.h>
#include <vtkGraph.h>
#include <vtkImageData.h>
#include <vtkProcessModule.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include <vtkSMClientDeliveryRepresentationProxy.h>
#include <vtkSMEnumerationDomain.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>

#include <QList>
#include <QSet>
#include <QHeaderView>
#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <QImage>

#include <QtDebug>

#include <vtksys/SystemTools.hxx>

/////////////////////////////////////////////////////////////////////////
// ChartSetupDialog::pqImplementation

class ChartSetupDialog::pqImplementation
{
public:
  pqImplementation(
    vtkTable *table) :
    Table(table),
    ColumnsAsSeries(true)
  {
  }

  ~pqImplementation()
  {
  }

  bool ColumnsAsSeries;
  QString XAxisColumnName;
  QString FirstDataColumnName;
  QString LastDataColumnName;

  Ui::ChartSetupDialog UI;
  vtkTable *Table;
};

/////////////////////////////////////////////////////////////////////////
// ChartSetupDialog

ChartSetupDialog::ChartSetupDialog(
  vtkTable *table,
  QWidget* widget_parent) :
    pqDialog(widget_parent),
    Implementation(new pqImplementation(table))
{
  this->Implementation->UI.setupUi(this);

  this->updateDisplay();

  QObject::connect(this->Implementation->UI.columnsAsSeries, SIGNAL(toggled(bool)),
    this, SLOT(updateDisplay()));
}

ChartSetupDialog::~ChartSetupDialog()
{
  delete this->Implementation;
}

bool ChartSetupDialog::viewColumnsAsSeries()
{
  return this->Implementation->UI.columnsAsSeries->isChecked();
}

QString ChartSetupDialog::xAxisColumn()
{
  return this->Implementation->UI.xAxisColumnName->currentText();
}

QString ChartSetupDialog::firstDataColumn()
{
  return this->Implementation->Table->GetColumnName(
      this->Implementation->UI.firstDataColumn->value());
}

QString ChartSetupDialog::lastDataColumn()
{
  return this->Implementation->Table->GetColumnName(
      this->Implementation->UI.lastDataColumn->value());
}

void ChartSetupDialog::updateDisplay()
{
  this->updateXAxisColumnWidget();
  this->updateDataColumnRangeWidget();
}

void ChartSetupDialog::updateXAxisColumnWidget()
{
  this->Implementation->UI.xAxisColumnName->clear();
  this->Implementation->UI.xAxisColumnName->addItem("None");
  if(this->Implementation->ColumnsAsSeries)
    {
    for(vtkIdType i=0; i<this->Implementation->Table->GetNumberOfColumns(); i++)
      {
      this->Implementation->UI.xAxisColumnName->addItem(this->Implementation->Table->GetColumnName(i));
      this->Implementation->UI.xAxisColumnName->setEnabled(true);
      }
    }
  else
    {
    this->Implementation->UI.xAxisColumnName->setEnabled(false);
    }
}

void ChartSetupDialog::updateDataColumnRangeWidget()
{
  if(this->Implementation->ColumnsAsSeries)
    {
    this->Implementation->UI.firstDataColumn->setMinimum(0);
    this->Implementation->UI.firstDataColumn->setMaximum(this->Implementation->Table->GetNumberOfColumns()-1);
    this->Implementation->UI.firstDataColumn->setValue(0);
    this->Implementation->UI.lastDataColumn->setMinimum(0);
    this->Implementation->UI.lastDataColumn->setMaximum(this->Implementation->Table->GetNumberOfColumns()-1);
    this->Implementation->UI.lastDataColumn->setValue(this->Implementation->Table->GetNumberOfColumns()-1);
    }
  else
    {
    this->Implementation->UI.firstDataColumn->setMinimum(0);
    this->Implementation->UI.firstDataColumn->setMaximum(this->Implementation->Table->GetNumberOfRows()-1);
    this->Implementation->UI.firstDataColumn->setValue(0);
    this->Implementation->UI.lastDataColumn->setMinimum(0);
    this->Implementation->UI.lastDataColumn->setMaximum(this->Implementation->Table->GetNumberOfRows()-1);
    this->Implementation->UI.lastDataColumn->setValue(this->Implementation->Table->GetNumberOfRows()-1);
    }
}
