/*=========================================================================

   Program: ParaView
   Module:    pqDataInformationModel.h

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
#ifndef __pqDataInformationModel_h
#define __pqDataInformationModel_h

#include "pqComponentsExport.h"
#include <QAbstractTableModel>

class pqPipelineSource;
class pqDataInformationModelInternal;

class PQCOMPONENTS_EXPORT pqDataInformationModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  pqDataInformationModel(QObject* _parent=NULL);
  virtual ~pqDataInformationModel();

  // QAbstractTableModel API. 
  // Returns the number of rows under the given parent.
  virtual int rowCount(const QModelIndex& parent =QModelIndex()) const;

  // QAbstractTableModel API.
  // Returns the number of columns for the given parent.
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;


  // QAbstractTableModel API.
  // Returns the data stored under the given role for the item referred 
  // to by the index.
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  
  // QAbstractTableModel API.
  // Returns the data for the given role and section in the header with the 
  // specified orientation.
  virtual QVariant headerData(int section, Qt::Orientation orientation, 
    int role = Qt::DisplayRole) const;

  // Given a pqPipelineSource, get the index for it, if present in this model,
  // otherwise returns invalid index.
  QModelIndex getIndexFor(pqPipelineSource* item) const;

  // Given a valid index, returns the pqPipelineSource item corresponding
  // to it.
  pqPipelineSource* getItemFor(const QModelIndex& index) const;

public slots:
  // Called when a new source/filter is registered.
  void addSource(pqPipelineSource* source);

  // Called when a new source/filter is unregistred.
  void removeSource(pqPipelineSource* source);

  // Called to make the model scan through the data information for
  // all know sources and update data if information for any source changed.
  void refreshModifiedData();

private:
  pqDataInformationModelInternal* Internal;

  enum ColumnType
    {
    Name=0,
    DataType,
    CellCount,
    PointCount,
    MemorySize,
    Bounds
    };
};


#endif

