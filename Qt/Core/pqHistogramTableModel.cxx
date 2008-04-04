/*=========================================================================

   Program: ParaView
   Module:    pqHistogramTableModel.cxx

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
#include "pqHistogramTableModel.h"

#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkSmartPointer.h>

#include <assert.h>

class pqHistogramTableModel::pqImplementation
{
public:
  pqImplementation(vtkDoubleArray* bin_extents, vtkIntArray* bin_values) :
    BinExtents(bin_extents),
    BinValues(bin_values)
  {
    assert(bin_extents);
    assert(bin_values);
    assert(bin_extents->GetNumberOfTuples() == (bin_values->GetNumberOfTuples() + 1));
  }
  
  vtkSmartPointer<vtkDoubleArray> BinExtents;
  vtkSmartPointer<vtkIntArray> BinValues;
};

pqHistogramTableModel::pqHistogramTableModel(vtkDoubleArray* bin_extents, vtkIntArray* bin_values, QObject* parent_object) :
  Superclass(parent_object),
  Implementation(new pqImplementation(bin_extents, bin_values))
{
}

pqHistogramTableModel::~pqHistogramTableModel()
{
  delete this->Implementation;
}

int pqHistogramTableModel::rowCount(const QModelIndex& /*parent*/) const
{
  return this->Implementation->BinValues->GetNumberOfTuples();
}

int pqHistogramTableModel::columnCount(const QModelIndex& /*parent*/) const
{
  return 3;
}

QVariant pqHistogramTableModel::data(const QModelIndex& model_index, int role) const
{
  if(role == Qt::DisplayRole)
    {
    switch(model_index.column())
      {
      case 0:
        return QString::number(this->Implementation->BinExtents->GetValue(model_index.row()));
      case 1:
        return QString::number(this->Implementation->BinExtents->GetValue(model_index.row() + 1));
      case 2:
        return QString::number(this->Implementation->BinValues->GetValue(model_index.row()));
      }
    }
    
  return QVariant();
}

QVariant pqHistogramTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
    switch(section)
      {
      case 0:
        return QString(tr("Bin min"));
      case 1:
        return QString(tr("Bin max"));
      case 2:
        return QString(tr("Bin count"));
      }
    }

  return QVariant();
}
