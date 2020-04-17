/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewSelectionModel.h

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

========================================================================*/
#ifndef pqSpreadSheetViewSelectionModel_h
#define pqSpreadSheetViewSelectionModel_h

#include "pqCoreModule.h"
#include <QItemSelectionModel>

class pqSpreadSheetViewModel;
class vtkSMSourceProxy;

/**
* This is the selection model used by spread sheet view. It manages two
* operations:
* \li When the QItemSelectionModel is updated by the QAbstractItemView
*     due to user interaction, pqSpreadSheetViewModel::select() gets called.
*     In that overload, this class creates a 'ParaView Selection' i.e. create a
*     selection source proxy for an ID based selection and set it as the
*     selection-input (vtkSMSourceProxy::SetSelectionInput) on the
*     data-source being shown in the view.
* \li Whenever the pqSpreadSheetViewModel receives new selection data from the
*     data-server, it updates its internal QItemSelection and fires
*     pqSpreadSheetViewModel::selectionChanged signal.
*     pqSpreadSheetViewSelectionModel handles that signal by updating itself to
*     mark the corresponding elements as selected.
*/
class PQCORE_EXPORT pqSpreadSheetViewSelectionModel : public QItemSelectionModel
{
  Q_OBJECT
  typedef QItemSelectionModel Superclass;

public:
  pqSpreadSheetViewSelectionModel(pqSpreadSheetViewModel* model, QObject* parent = 0);
  ~pqSpreadSheetViewSelectionModel() override;

public Q_SLOTS:
  void select(const QModelIndex& index, QItemSelectionModel::SelectionFlags command) override
  {
    this->Superclass::select(index, command);
  }

  void select(
    const QItemSelection& selection, QItemSelectionModel::SelectionFlags command) override;

Q_SIGNALS:
  void selection(vtkSMSourceProxy*);

protected Q_SLOTS:
  void serverSelectionChanged(const QItemSelection&);

protected:
  /**
  * Locate the selection source currently set on the representation being shown.
  * If no selection exists, or selection present is not "updatable" by this
  * model, we create a new selection.
  */
  vtkSMSourceProxy* getSelectionSource();

  bool UpdatingSelection;

private:
  Q_DISABLE_COPY(pqSpreadSheetViewSelectionModel)

  class pqInternal;
  pqInternal* Internal;
};

#endif
