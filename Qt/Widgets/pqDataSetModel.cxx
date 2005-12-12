
#include "pqDataSetModel.h"

#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkCellData.h>

pqDataSetModel::pqDataSetModel(QObject* p)
  : QAbstractTableModel(p), DataSet(0)
{
}

pqDataSetModel::~pqDataSetModel()
{
  this->setDataSet(0);
}


int pqDataSetModel::rowCount(const QModelIndex&) const
{
  if(!this->DataSet)
    {
    return 0;
    }
  return this->DataSet->GetNumberOfCells();
}

int pqDataSetModel::columnCount(const QModelIndex&) const
{
  if(!this->DataSet)
    {
    return 0;
    }

  return this->DataSet->GetCellData()->GetNumberOfArrays();
}

QVariant pqDataSetModel::data(const QModelIndex& index, int role) const
{
  if(!index.isValid() || !this->DataSet)
    {
    return QVariant();
    }

  vtkDataArray* array = this->DataSet->GetCellData()->GetArray(index.column());

  if(role == Qt::DisplayRole)
    {
    return array->GetTuple1(index.row());
    }
  
  return QVariant();
}

QVariant pqDataSetModel::headerData(int section, Qt::Orientation, int role) const
{
  if(!this->DataSet)
    {
    return QVariant();
    }

  if(role == Qt::DisplayRole)
    {
    vtkDataArray* array = this->DataSet->GetCellData()->GetArray(section);
    return array->GetName();
    }

  return QVariant();
}


void pqDataSetModel::setDataSet(vtkDataSet* ds)
{
  if(ds == this->DataSet)
    {
    return;
    }

  int oldRow=0, oldCol=0;
  int newRow=0, newCol=0;

  if(this->DataSet)
    {
    oldRow = this->DataSet->GetNumberOfCells();
    oldCol = this->DataSet->GetCellData()->GetNumberOfArrays();
    this->DataSet->UnRegister(NULL);
    }

  this->DataSet = ds;

  if(this->DataSet)
    {
    this->DataSet->Register(NULL);
    newRow = this->DataSet->GetNumberOfCells();
    newCol = this->DataSet->GetCellData()->GetNumberOfArrays();
    }
  
  // TODO : tell the view that we changed
}

vtkDataSet* pqDataSetModel::dataSet() const
{
  return this->DataSet;
}


