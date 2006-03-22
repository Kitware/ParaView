
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
  if(this->DataSet)
    {
    this->DataSet->UnRegister(NULL);
    }
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

QVariant pqDataSetModel::data(const QModelIndex& idx, int role) const
{
  if(!idx.isValid() || !this->DataSet)
    {
    return QVariant();
    }

  vtkDataArray* array = this->DataSet->GetCellData()->GetArray(idx.column());

  if(role == Qt::DisplayRole)
    {
    return array->GetTuple1(idx.row());
    }
  
  return QVariant();
}

QVariant pqDataSetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(this->DataSet && orientation == Qt::Horizontal)
    {
    if(role == Qt::DisplayRole)
      {
      vtkDataArray* array = this->DataSet->GetCellData()->GetArray(section);
      return array->GetName();
      }
    }

  return QVariant();
}


void pqDataSetModel::setDataSet(vtkDataSet* ds)
{
  if(ds == this->DataSet)
    {
    return;
    }

  if(this->DataSet)
    {
    this->DataSet->UnRegister(NULL);
    }

  this->DataSet = ds;

  if(this->DataSet)
    {
    this->DataSet->Register(NULL);
    }
  
  // Tell the view that we changed.
  this->reset();
}

vtkDataSet* pqDataSetModel::dataSet() const
{
  return this->DataSet;
}


