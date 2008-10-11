#include "TableToSparseArrayPanel.h"

#include <pqProxy.h>
#include <pqPropertyHelper.h>

#include <vtkSMProxy.h>

#include <QLabel>
#include <QLayout>
#include <QMessageBox>

#include <iostream>
#include <set>

TableToSparseArrayPanel::TableToSparseArrayPanel(pqProxy* proxy, QWidget* p) :
  pqObjectPanel(proxy, p)
{
  this->Widgets.setupUi(this);

  vtkSMProxy* const table_to_sparse_array = proxy->getProxy();

  const QStringList fields = pqPropertyHelper(table_to_sparse_array, "CoordinateColumns").GetAsStringList();
  if(fields.size() > 0)
    {
    this->Widgets.coordinateColumn1->setText(fields[0]);
    }
  if(fields.size() > 1)
    {
    this->Widgets.coordinateColumn2->setText(fields[1]);
    }
  if(fields.size() > 2)
    {
    this->Widgets.coordinateColumn3->setText(fields[2]);
    }
  if(fields.size() > 3)
    {
    this->Widgets.coordinateColumn4->setText(fields[3]);
    }

  this->Widgets.valueColumn->setText(vtkSMPropertyHelper(table_to_sparse_array, "ValueColumn").GetAsString());

  QObject::connect(this->Widgets.coordinateColumn1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.coordinateColumn2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.coordinateColumn3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.coordinateColumn4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.valueColumn, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
}

void TableToSparseArrayPanel::accept()
{
  QStringList coordinate_columns;
  if(!this->Widgets.coordinateColumn1->text().isEmpty())
    coordinate_columns << this->Widgets.coordinateColumn1->text();
  if(!this->Widgets.coordinateColumn2->text().isEmpty())
    coordinate_columns << this->Widgets.coordinateColumn2->text();
  if(!this->Widgets.coordinateColumn3->text().isEmpty())
    coordinate_columns << this->Widgets.coordinateColumn3->text();
  if(!this->Widgets.coordinateColumn4->text().isEmpty())
    coordinate_columns << this->Widgets.coordinateColumn4->text();

  vtkSMProxy* const table_to_sparse_array = this->referenceProxy()->getProxy();
  pqPropertyHelper(table_to_sparse_array, "CoordinateColumns").Set(coordinate_columns);
  vtkSMPropertyHelper(table_to_sparse_array, "ValueColumn").Set(this->Widgets.valueColumn->text().toAscii().data());
  table_to_sparse_array->UpdateVTKObjects();

  Superclass::accept();    
}

void TableToSparseArrayPanel::reset()
{
  Superclass::reset();
}

