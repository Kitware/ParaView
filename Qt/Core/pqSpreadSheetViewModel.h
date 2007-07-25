/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewModel.h

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
#ifndef __pqSpreadSheetViewModel_h 
#define __pqSpreadSheetViewModel_h

#include <QAbstractTableModel>
#include "pqCoreExport.h"

class vtkSMBlockDeliveryRepresentationProxy;
class PQCORE_EXPORT pqSpreadSheetViewModel : public QAbstractTableModel
{
  Q_OBJECT
  typedef QAbstractTableModel Superclass;
public:
  pqSpreadSheetViewModel();
  ~pqSpreadSheetViewModel();

  /// Returns the number of rows.
  int rowCount(const QModelIndex& parent=QModelIndex()) const;

  /// Returns the number of columns.
  int columnCount(const QModelIndex& parent=QModelIndex()) const;

  /// Returns the data storeed under the given role for the item referred by the
  /// index.
  QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;

  /// Returns the data for the given role and section in the header with the
  /// specified orientation.
  QVariant headerData (int section, Qt::Orientation orientation, 
    int role=Qt::DisplayRole) const; 

  /// Set/Get the representation proxy which is currently displayed in this
  /// model.
  void setRepresentationProxy(vtkSMBlockDeliveryRepresentationProxy*);
  vtkSMBlockDeliveryRepresentationProxy* getRepresentationProxy() const;

  /// resets the model.
  void forceUpdate();

  /// Set the best estimate for the visible block. The model will request data
  /// (if not available) only for the most recently selected active block.
  void setActiveBlock(QModelIndex top, QModelIndex bottom);
signals:
  void requestDelayedUpdate() const;

private slots:
  void delayedUpdate();

private:
  pqSpreadSheetViewModel(const pqSpreadSheetViewModel&); // Not implemented.
  void operator=(const pqSpreadSheetViewModel&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif


