/*=========================================================================

   Program: ParaView
   Module:    pqDataSetModel.h

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


#ifndef _pqDataSetModel_h
#define _pqDataSetModel_h

#include "pqWidgetsExport.h"
#include <QAbstractTableModel>
class vtkDataSet;

/// provide a QAbstractTableModel for a vtkDataSet's cell scalars
/// \ todo fix this class to watch for changes in the pipeline and update the view accordingly
class PQWIDGETS_EXPORT pqDataSetModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  pqDataSetModel(QObject* p);
  ~pqDataSetModel();

  /// return the number of rows (number of cells in dataset)
  int rowCount(const QModelIndex& p = QModelIndex()) const;
  /// return number of columns (number of cell data arrays)
  int columnCount(const QModelIndex& p = QModelIndex()) const;
  /// return data for a position in the table
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  /// return the column headers (names of cell data arrays)
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  /// set the vtkDataSet to use
  void setDataSet(vtkDataSet* ds);
  /// get the vtkDataSet in use
  vtkDataSet* dataSet() const;

private:
  vtkDataSet* DataSet;

};

#endif //_pqDataSetModel_h

