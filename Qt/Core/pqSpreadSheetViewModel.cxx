/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewModel.cxx

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

========================================================================*/
#include "pqSpreadSheetViewModel.h"

// Server Manager Includes.
#include "vtkIndexBasedBlockFilter.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMBlockDeliveryRepresentationProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariant.h"

// Qt Includes.
#include <QSet>
#include <QTimer>

// ParaView Includes.
#include "pqSMAdaptor.h"

class pqSpreadSheetViewModel::pqInternal
{
public:
  pqInternal() : NumberOfColumns(0), NumberOfRows(0)
  {
  }
  vtkSmartPointer<vtkSMBlockDeliveryRepresentationProxy> Representation;
  int NumberOfColumns;
  int NumberOfRows;

  vtkIdType getBlockSize()
    {
    vtkIdType blocksize = pqSMAdaptor::getElementProperty(
      this->Representation->GetProperty("BlockSize")).value<vtkIdType>();
    return blocksize;
    }

  // Computes the block number given the row number.
  vtkIdType computeBlockNumber(int row)
    {
    vtkIdType blocksize = this->getBlockSize();
    return row/blocksize;
    }

  // Computes the index for the row. 
  vtkIdType computeBlockOffset(int row)
    {
    vtkIdType blocksize = this->getBlockSize(); 
    return (row % blocksize);
    }

  QSet<vtkIdType> PendingBlocks;
  QTimer Timer;
};

//-----------------------------------------------------------------------------
pqSpreadSheetViewModel::pqSpreadSheetViewModel()
{
  this->Internal = new pqInternal();
  
  this->Internal->Timer.setSingleShot(true);
  this->Internal->Timer.setInterval(100);//milliseconds.
  QObject::connect(&this->Internal->Timer, SIGNAL(timeout()),
    this, SLOT(delayedUpdate()));
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewModel::~pqSpreadSheetViewModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
int pqSpreadSheetViewModel::rowCount(const QModelIndex&) const
{
  return this->Internal->NumberOfRows;
}

//-----------------------------------------------------------------------------
int pqSpreadSheetViewModel::columnCount(const QModelIndex&) const
{
  return this->Internal->NumberOfColumns;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::setRepresentationProxy(
  vtkSMBlockDeliveryRepresentationProxy* repr)
{
  if (this->Internal->Representation.GetPointer() != repr)
    {
    this->Internal->Representation = repr;
    }
}

//-----------------------------------------------------------------------------
vtkSMBlockDeliveryRepresentationProxy* pqSpreadSheetViewModel::
getRepresentationProxy() const
{
  return this->Internal->Representation;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::forceUpdate()
{
  // Note that this method is called after the representation has already been
  // updated.
  this->Internal->NumberOfRows = 0;
  this->Internal->NumberOfColumns = 0;
  vtkSMBlockDeliveryRepresentationProxy* repr = this->Internal->Representation;
  if (repr)
    {
    vtkTable* table = vtkTable::SafeDownCast(repr->GetOutput());
    int field_type = pqSMAdaptor::getElementProperty(
      repr->GetProperty("FieldType")).toInt();

    vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      repr->GetProperty("Input"));
    vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(
      ip->GetProxy(0));
    int port = ip->GetOutputPortForConnection(0);

    vtkPVDataInformation* info = inputProxy?
      inputProxy->GetDataInformation(port) : 0;
    if (info)
      {
      if (field_type == vtkIndexBasedBlockFilter::DATA_OBJECT_FIELD)
        {
        // TODO:
        }
      else if (field_type == vtkIndexBasedBlockFilter::POINT_DATA_FIELD)
        {
        this->Internal->NumberOfRows = info->GetNumberOfPoints();
        }
      else if (field_type == vtkIndexBasedBlockFilter::CELL_DATA_FIELD)
        {
        this->Internal->NumberOfRows = info->GetNumberOfCells();
        }
      }
    this->Internal->NumberOfColumns = table? table->GetNumberOfColumns()  :0;
    }
  this->reset();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::delayedUpdate()
{
  vtkSMBlockDeliveryRepresentationProxy* repr = 
    this->Internal->Representation;
  if (repr)
    {
    QModelIndex topLeft;
    QModelIndex bottomRight;
    vtkIdType blocksize = this->Internal->getBlockSize();
    foreach (vtkIdType blockNumber, this->Internal->PendingBlocks)
      {
      // cout << "Requesting : " << blockNumber << endl;
      pqSMAdaptor::setElementProperty(repr->GetProperty("Block"), blockNumber);
      repr->UpdateProperty("Block");
      repr->Update();
      QModelIndex myTopLeft(this->index(blockNumber*blocksize, 0));
      int botRow = blocksize*(blockNumber+1);
      botRow = (botRow<this->rowCount())? botRow: this->rowCount()-1;
      QModelIndex myBottomRight(this->index(botRow, this->columnCount()-1));
      topLeft = (topLeft.isValid() && topLeft < myTopLeft)?  topLeft : myTopLeft;
      bottomRight = (bottomRight.isValid() && myBottomRight<bottomRight)? 
        bottomRight:myBottomRight;
      }
    if (topLeft.isValid() && bottomRight.isValid())
      {
      this->dataChanged(topLeft, bottomRight);

      // we always invalidate header data, just to be on a safe side.
      this->headerDataChanged(Qt::Horizontal, 0, this->columnCount()-1);
      }
    }
  this->Internal->PendingBlocks.clear();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::setActiveBlock(QModelIndex top, QModelIndex bottom)
{
  this->Internal->PendingBlocks.clear();
  if (this->Internal->Representation)
    {
    vtkIdType topBlock = this->Internal->computeBlockNumber(top.row());
    vtkIdType bottomBlock = this->Internal->computeBlockNumber(bottom.row());
    for (vtkIdType cc=topBlock; cc <= bottomBlock; cc++)
      {
      this->Internal->PendingBlocks.insert(cc);
      }
    }
}

//-----------------------------------------------------------------------------
QVariant pqSpreadSheetViewModel::data(
  const QModelIndex& idx, int role/*=Qt::DisplayRole*/) const
{
  vtkSMBlockDeliveryRepresentationProxy* repr = 
    this->Internal->Representation;
  int row = idx.row();
  int column = idx.column();
  if (role == Qt::DisplayRole && repr)
    {
    vtkIdType blockNumber = this->Internal->computeBlockNumber(row);
    vtkIdType blockOffset = this->Internal->computeBlockOffset(row);
    // cout << row << " " << "blockNumber: " << blockNumber << endl;
    // cout << row << " " << "blockOffset: " << blockOffset << endl;

    if (!repr->IsCached(blockNumber))
      {
      this->Internal->PendingBlocks.insert(blockNumber);
      this->Internal->Timer.start();
      return QVariant("...");
      }

    pqSMAdaptor::setElementProperty(repr->GetProperty("Block"), blockNumber);
    repr->UpdateProperty("Block");
    vtkTable* table = vtkTable::SafeDownCast(repr->GetBlockOutput());
    if (table)
      {
      vtkVariant value = table->GetValue(blockOffset, column);
      return QVariant(value.ToString().c_str());
      }
    }

  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant pqSpreadSheetViewModel::headerData (int section, Qt::Orientation orientation, 
    int role/*=Qt::DisplayRole*/) const 
{
  vtkSMBlockDeliveryRepresentationProxy* repr = 
    this->Internal->Representation;
  if (orientation == Qt::Horizontal && repr && role == Qt::DisplayRole)
    {
    // No need to get updated data, simply get the current data.
    vtkTable* table = vtkTable::SafeDownCast(repr->GetOutput());
    if (table && table->GetNumberOfColumns() > section)
      {
      QString title = table->GetColumnName(section);
      // Convert names of some standard arrays to user-friendly ones.
      if (title == "vtkOriginalProcessIds")
        {
        title = "Process ID";
        }
      else if (title == "vtkOriginalPointIds")
        {
        title = "Point ID";
        }
      else if (title == "vtkOriginalCellIds")
        {
        title = "Cell ID";
        }
      return QVariant(title);
      }
    }

  return this->Superclass::headerData(section, orientation, role);
}
