/*=========================================================================

   Program:   ParaQ
   Module:    pqSelectionAdaptor.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
#ifndef __pqSelectionAdaptor_h
#define __pqSelectionAdaptor_h

#include "pqWidgetsExport.h"
#include <QObject>

class pqSelectionAdaptorInternal;
class pqServerManagerModelItem;
class pqServerManagerModelSelection;
class pqServerManagerSelectionModel;

class QItemSelection;
class QItemSelectionModel;
class QModelIndex;

// pqSelectionAdaptor is the abstract base class for an adaptor that connects a
// QItemSelectionModel for any QAbstractItemModel to a 
// pqServerManagerSelectionModel. When the selection in the QItemSelectionModel
// changes, the pqServerManagerSelectionModel will be updated and vice versa.
// Every model implemented on top of pqServerManagerModel that should
// participate in synchronized selections would typically subclass and implement 
// an adaptor. Subclass typically only need to implement
// mapToSMModel() and mapFromSMModel().
class PQWIDGETS_EXPORT pqSelectionAdaptor : public QObject
{
  Q_OBJECT

public:
  virtual ~pqSelectionAdaptor();

  // Returns a pointer to the QItemSelectionModel.
  QItemSelectionModel* getQSelectionModel() const;

  // Reurns a pointer to the pqServerManagerSelectionModel.
  pqServerManagerSelectionModel* getSMSelectionModel() const;

protected:
  pqSelectionAdaptor(QItemSelectionModel* pipelineSelectionModel,
    pqServerManagerSelectionModel* smSelectionModel, QObject* parent=0);

  // Maps a pqServerManagerModelItem to an index in the QAbstractItemModel.
  // Subclass must implement this method.
  virtual QModelIndex mapFromSMModel(pqServerManagerModelItem* item) const = 0;

  // Maps a QModelIndex to a pqServerManagerModelItem.
  // Subclass must implement this method.
  virtual pqServerManagerModelItem* mapToSMModel(
    const QModelIndex& index) const =0;

protected slots:
  virtual void currentChanged(const QModelIndex& current, 
    const QModelIndex& previous);
  virtual void selectionChanged(const QItemSelection& selected,
    const QItemSelection& deselected);

  virtual void currentChanged(pqServerManagerModelItem* item);
  virtual void selectionChanged(const pqServerManagerModelSelection& selected,
    const pqServerManagerModelSelection& deselected);
  
private:
  pqSelectionAdaptorInternal* Internal;
};


#endif

