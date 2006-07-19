/*=========================================================================

   Program: ParaView
   Module:    pqScalarSetModel.cxx

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

#include "pqScalarSetModel.h"

#include <vtkstd/set>

////////////////////////////////////////////////////////////////////////////
// pqScalarSetModel::pqImplementation

class pqScalarSetModel::pqImplementation
{
public:
  pqImplementation() :
    Format('g'),
    Precision(3)
  {
  }
  
  vtkstd::set<double> Values;
  char Format;
  int Precision;
};

/////////////////////////////////////////////////////////////////////////////
// pqScalarSetModel

pqScalarSetModel::pqScalarSetModel() :
  Implementation(new pqImplementation())
{
}

pqScalarSetModel::~pqScalarSetModel()
{
  delete this->Implementation;
}

void pqScalarSetModel::clear()
{
  this->Implementation->Values.clear();
  emit layoutChanged();
}

void pqScalarSetModel::insert(double value)
{
  this->Implementation->Values.insert(value);
  emit layoutChanged();
}

void pqScalarSetModel::erase(double value)
{
  this->Implementation->Values.erase(value);
  emit layoutChanged();
}

void pqScalarSetModel::erase(int row)
{
  if(row < 0 || row >= static_cast<int>(this->Implementation->Values.size()))
    return;
    
  vtkstd::set<double>::iterator iterator = this->Implementation->Values.begin();
  vtkstd::advance(iterator, row);
  this->Implementation->Values.erase(iterator);
  emit layoutChanged();
}

const QList<double> pqScalarSetModel::values()
{
  QList<double> results;
  
  vtkstd::copy(
    this->Implementation->Values.begin(),
    this->Implementation->Values.end(),
    vtkstd::back_inserter(results));
  
  return results;
}

void pqScalarSetModel::setFormat(char f, int precision)
{
  this->Implementation->Format = f;
  this->Implementation->Precision = precision;
  emit dataChanged(this->index(0), this->index(this->Implementation->Values.size() - 1));
}

QVariant pqScalarSetModel::data(const QModelIndex& i, int role) const
{
  if(!i.isValid())
    return QVariant();
  
  if(i.row() < 0 || i.row() >= static_cast<int>(this->Implementation->Values.size()))
    return QVariant();
  
  switch(role)
    {
    case Qt::DisplayRole:
      {
      vtkstd::set<double>::iterator iterator = this->Implementation->Values.begin();
      vtkstd::advance(iterator, i.row());
      return QString::number(
        *iterator, this->Implementation->Format, this->Implementation->Precision);
      }
    }
    
  return QVariant();
}

Qt::ItemFlags pqScalarSetModel::flags(const QModelIndex& /*i*/) const
{
  return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int pqScalarSetModel::rowCount(const QModelIndex& /*parent*/) const
{
  return Implementation->Values.size();
}

bool pqScalarSetModel::setData(const QModelIndex& i, const QVariant& value, int role)
{
  if(!i.isValid())
    return false;
    
  if(i.row() < 0 || i.row() >= static_cast<int>(this->Implementation->Values.size()))
    return false;
  
  switch(role)
    {
    case Qt::EditRole:
      {
      vtkstd::set<double>::iterator iterator = this->Implementation->Values.begin();
      vtkstd::advance(iterator, i.row());
      this->Implementation->Values.erase(iterator);
      this->Implementation->Values.insert(value.toDouble());
      emit dataChanged(this->index(0), this->index(this->Implementation->Values.size() - 1));
      emit layoutChanged();
      }
    }
    
  return true;
}
