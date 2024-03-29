// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpreadSheetViewModel_h
#define pqSpreadSheetViewModel_h

#include "pqCoreModule.h"
#include "vtkTuple.h" // needed for vtkTuple.
#include "vtkType.h"  // needed for vtkIdType.
#include <QAbstractTableModel>
#include <QPair>
#include <QSet>

/**
 * This is the model used by SpreadSheetView to show the data. This model works
 * with vtkSMSpreadSheetRepresentationProxy to fetch blocks of data from the
 * server and show them. It requires that vtkSMSpreadSheetRepresentationProxy
 * delivers vtkTable.
 */
class pqDataRepresentation;
class QItemSelection;
class QItemSelectionModel;
class vtkObject;
class vtkSelection;
class vtkSelectionNode;
class vtkSMProxy;
class vtkSpreadSheetView;

class PQCORE_EXPORT pqSpreadSheetViewModel : public QAbstractTableModel
{
  Q_OBJECT
  typedef QAbstractTableModel Superclass;

public:
  pqSpreadSheetViewModel(vtkSMProxy* viewProxy, QObject* parent = nullptr);
  ~pqSpreadSheetViewModel() override;

  class vtkIndex : public vtkTuple<vtkIdType, 3>
  {
  };

  /**
   * Returns the number of rows.
   */
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
   * Returns the number of columns.
   */
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
   * Returns the data storeed under the given role for the item referred by the
   * index.
   */
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /**
   * Returns the data for the given role and section in the header with the
   * specified orientation.
   *
   * When orientation is Qt::Horizontal, pqSpreadSheetViewModel adds support for
   * two new roles: `pqSpreadSheetViewModel::SectionVisible`, and
   * `pqSpreadSheetViewModel::SectionInternal`. `SectionVisible` can be used to
   * determine if the column (or horizontal section) is visible or hidden.
   * `SectionInternal` is used to determine if the section is internal. An
   * internal section is also hidden.
   * When data associated with these two roles changes, the model will fire
   * `headerDataChanged` signal.
   */
  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  /**
   * Make a server request to sort based on a given column with a given order
   */
  void sortSection(int section, Qt::SortOrder order);

  /**
   * Return true only if the given column is sortable.
   */
  bool isSortable(int section);

  /**
   * Returns the field type for the data currently shown by this model.
   */
  int getFieldType() const;

  // Returns the vtk indices for the view indices.
  QSet<vtkIndex> getVTKIndices(const QModelIndexList& indexes);

  /**
   * Set/Get the decimal precision for float and double type data.
   */
  void setDecimalPrecision(int);
  int getDecimalPrecision();

  /** Set/Get whether the decimal representation is fixed or scientific.  True
   * is fixed and False is scientific
   */
  void setFixedRepresentation(bool);
  bool getFixedRepresentation();

  /**
   * set the region (in row indices) that is currently being shown in the view.
   * the model will provide data-values only for the active-region. For any
   * other region it will simply return a "..." text for display (in
   * QAbstractTableModel::data(..) callback).
   */
  void setActiveRegion(int row_top, int row_bottom);

  /**
   * Returns the active representation. Active representation is the
   * representation being shown by the view.
   */
  pqDataRepresentation* activeRepresentation() const;
  vtkSMProxy* activeRepresentationProxy() const;

  /**
   * Method needed for copy/past cell editor
   */
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

  enum
  {
    SectionInternal = Qt::UserRole + 1,
    SectionVisible,
  };

  /**
   * Returns the current rows as a CSV-formatted string, including header with column names.
   * Uses the vtkCSVExporter.
   */
  QString GetRowsAsString() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * resets the model.
   */
  void forceUpdate();

  /**
   * Sets the active representation. Active representation is the
   * representation being shown by the view.
   */
  void setActiveRepresentation(pqDataRepresentation*);

  /**
   * Sets the active representation. When using this API instead of
   * setActiveRepresentation(pqDataRepresentation*), some functionality won't be
   * available.
   */
  void setActiveRepresentationProxy(vtkSMProxy*);

Q_SIGNALS:
  /**
   * Fired whenever the server side selection changes.
   */
  void selectionChanged(const QItemSelection& selection);

private Q_SLOTS:
  /**
   * called to fetch data for all pending blocks.
   */
  void delayedUpdate();

  void triggerSelectionChanged();

  /**
   * Called when the vtkSpreadSheetView fetches a new block, we fire
   * dataChanged signal.
   */
  void onDataFetched(vtkObject*, unsigned long, void*, void* call_data);

  /**
   * Called when "HiddenColumnLabels" property is modified.
   */
  void hiddenColumnsChanged();

protected:
  /**
   * Given an index into the model, check to see that its row number is
   * less than the length of the data array associated with its column
   */
  bool isDataValid(const QModelIndex& idx) const;

  vtkSpreadSheetView* GetView() const;

private:
  Q_DISABLE_COPY(pqSpreadSheetViewModel)

  class pqInternal;
  pqInternal* Internal;

  vtkSMProxy* ViewProxy;
};

#endif
