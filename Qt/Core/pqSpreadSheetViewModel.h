/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewModel.h

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
#ifndef pqSpreadSheetViewModel_h
#define pqSpreadSheetViewModel_h

#include "pqCoreModule.h"
#include "vtkType.h" // needed for vtkIdType.
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

  class vtkIndex
  {
  public:
    vtkIdType Tuple[3];
    vtkIndex()
    {
      this->Tuple[0] = 0;
      this->Tuple[1] = 0;
      this->Tuple[2] = 0;
    }

    vtkIndex(vtkIdType a, vtkIdType b, vtkIdType c)
    {
      this->Tuple[0] = a;
      this->Tuple[1] = b;
      this->Tuple[2] = c;
    }

    bool operator==(const vtkIndex& other) const
    {
      return (this->Tuple[0] == other.Tuple[0] && this->Tuple[1] == other.Tuple[1] &&
        this->Tuple[2] == other.Tuple[2]);
    }
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

  /** Set/Get whether the decimal representation is fixed or
  * scientific.  True is fixed and False is scientific
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

public Q_SLOTS:
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
