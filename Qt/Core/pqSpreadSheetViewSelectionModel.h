// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  pqSpreadSheetViewSelectionModel(pqSpreadSheetViewModel* model, QObject* parent = nullptr);
  ~pqSpreadSheetViewSelectionModel() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
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

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Locate the selection source currently set on the representation being shown.
   * If no selection exists, or selection present is not "updatable" by this
   * model, we create a new selection.
   */
  vtkSMSourceProxy* getSelectionSource();

  bool UpdatingSelection;

private:
  Q_DISABLE_COPY(pqSpreadSheetViewSelectionModel)

  pqSpreadSheetViewModel* Model;
};

#endif
