/*=========================================================================

   Program: ParaView
   Module:    pqDataInformationModelSelectionAdaptor.h

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
#ifndef __pqDataInformationModelSelectionAdaptor_h
#define __pqDataInformationModelSelectionAdaptor_h

#include "pqSelectionAdaptor.h"

// pqDataInformationModelSelectionAdaptor is the adaptor that connects a
// QItemSelectionModel for a pqDataInformationModel to a 
// pqServerManagerSelectionModel. When the selection in the pqDataInformationModel
// changes, the pqServerManagerSelectionModel will be updated and vice versa.
// Selection adaptors are part of the "Synchronized selection" mechanism
// making it possible to different views connected to different models 
// which are based on pqServerManagerModel to coordinate the selection state.
class PQCOMPONENTS_EXPORT pqDataInformationModelSelectionAdaptor :
  public pqSelectionAdaptor
{
  Q_OBJECT

public:
  pqDataInformationModelSelectionAdaptor(QItemSelectionModel* pipelineSelectionModel,
    pqServerManagerSelectionModel* smSelectionModel, QObject* parent=0);
  virtual ~pqDataInformationModelSelectionAdaptor();

protected:
  // Maps a pqServerManagerModelItem to an index in the QAbstractItemModel.
  // Subclass must implement this method.
  QModelIndex mapFromSMModel( pqServerManagerModelItem* item) const;

  // Maps a QModelIndex to a pqServerManagerModelItem.
  // Subclass must implement this method.
  virtual pqServerManagerModelItem* mapToSMModel(
    const QModelIndex& index) const;

  // subclasses can override this method to provide model specific selection 
  // overrides such as QItemSelection::Rows or QItemSelection::Columns etc.
  virtual QItemSelectionModel::SelectionFlag qtSelectionFlags() const; 
};

#endif

