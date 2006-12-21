/*=========================================================================

   Program: ParaView
   Module:    pqDataSetModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqDataSetModel.h"

#include <vtkDataArray.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkStdString.h>

#include <QtDebug>
//-----------------------------------------------------------------------------
pqDataSetModel::pqDataSetModel(QObject* p)
  : QAbstractTableModel(p), DataSet(0)
{
  this->Type = pqDataSetModel::CELL_DATA_FIELD;
}

//-----------------------------------------------------------------------------
pqDataSetModel::~pqDataSetModel()
{
  if(this->DataSet)
    {
    this->DataSet->UnRegister(NULL);
    }
}

//-----------------------------------------------------------------------------
vtkFieldData* pqDataSetModel::getFieldData() const
{
 if (this->DataSet)
   {
   switch (this->Type)
     {
   case DATA_OBJECT_FIELD:
     return this->DataSet->GetFieldData();

   case POINT_DATA_FIELD:
     return this->DataSet->GetPointData();

   case CELL_DATA_FIELD:
     return this->DataSet->GetCellData();
     }
   }
  return 0;
}
//-----------------------------------------------------------------------------
int pqDataSetModel::rowCount(const QModelIndex&) const
{
  vtkFieldData* fd = this->getFieldData();
  return fd? fd->GetNumberOfTuples() : 0;
}

//-----------------------------------------------------------------------------
int pqDataSetModel::columnCount(const QModelIndex&) const
{
  vtkFieldData* fd = this->getFieldData();
  return fd? fd->GetNumberOfArrays() : 0;
}

//-----------------------------------------------------------------------------
template <class T>
void pqDataSetModelPrintTuple(QString& str, T *tuple, int num_of_components)
{
  for (int cc=0; cc < num_of_components; cc++)
    {
    if (cc > 0)
      {
      str += ", ";
      }
    str += QString::number(tuple[cc]);
    }
}
//-----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
void pqDataSetModelPrintTuple(QString& str, vtkStdString* tuple, int num_of_components)
{
  for (int cc=0; cc < num_of_components; cc++)
    {
    if (cc > 0)
      {
      str += ", ";
      }
    str += tuple[cc].c_str();
    }
}

//-----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
void pqDataSetModelPrintTuple(QString& str, double* tuple, int num_of_components)
{
  for (int cc=0; cc < num_of_components; cc++)
    {
    if (cc > 0)
      {
      str += ", ";
      }
    str += QString::number(tuple[cc],'g');;
    }
}
//-----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
void pqDataSetModelPrintTuple(QString& str, float* tuple, int num_of_components)
{
  for (int cc=0; cc < num_of_components; cc++)
    {
    if (cc > 0)
      {
      str += ", ";
      }
    str += QString::number(tuple[cc],'g');;
    }
}
//-----------------------------------------------------------------------------
QVariant pqDataSetModel::data(const QModelIndex& idx, int role) const
{
  if(!idx.isValid() || !this->DataSet)
    {
    return QVariant();
    }
  vtkFieldData* fd = this->getFieldData();
  vtkDataArray* array = fd?  fd->GetArray(idx.column()) : 0;

  if(role == Qt::DisplayRole && array)
    {
    QString text;
    int num_of_components = array->GetNumberOfComponents();
    switch (array->GetDataType())
      {
      vtkExtendedTemplateMacro(::pqDataSetModelPrintTuple(text,
          static_cast<VTK_TT*>(array->GetVoidPointer(idx.row()*num_of_components)),
          num_of_components));

    default:
      qDebug() << "Unsupported data type: " << array->GetDataType();
      }
    return text;
    }
  
  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant pqDataSetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(this->DataSet && orientation == Qt::Horizontal)
    {
    if(role == Qt::DisplayRole)
      {
      vtkFieldData* fd = this->getFieldData();
      vtkDataArray* array = fd?  fd->GetArray(section) : 0;
      return array? array->GetName() : QVariant();
      }
    }

  return QVariant();
}


//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
vtkDataSet* pqDataSetModel::dataSet() const
{
  return this->DataSet;
}

//-----------------------------------------------------------------------------
void pqDataSetModel::setFieldDataType(FieldDataType type)
{
  if (this->Type != type)
    {
    this->Type = type;
    // Tell the view that we changed.
    this->reset();
    }
}

