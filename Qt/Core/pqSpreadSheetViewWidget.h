// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpreadSheetViewWidget_h
#define pqSpreadSheetViewWidget_h

#include "pqCoreModule.h"
#include <QTableView>

class pqSpreadSheetViewModel;

/**
 * pqSpreadSheetViewWidget is a  QTableView subclass that is used by
 * pqSpreadSheetView as the widget showing the data. Although it's called a
 * 'Widget' it's not a QTableWidget but a QTableView subclass. It works with a
 * pqSpreadSheetViewModel to show raw data delivered by the
 * vtkSMSpreadSheetRepresentationProxy.
 *
 * pqSpreadSheetViewWidget uses an internal QItemDelegate subclass to handle
 * determining of the active viewport as well as showing multi-component
 * arrays. Users are advised not to change the item delegate on the view.
 */
class PQCORE_EXPORT pqSpreadSheetViewWidget : public QTableView
{
  Q_OBJECT
  typedef QTableView Superclass;

public:
  pqSpreadSheetViewWidget(QWidget* parent = nullptr);
  ~pqSpreadSheetViewWidget() override;

  /**
   * Overridden to ensure that the model is a pqSpreadSheetViewModel.
   */
  void setModel(QAbstractItemModel* model) override;

  /**
   * Returns the spread sheetview model for this view.
   */
  pqSpreadSheetViewModel* spreadSheetViewModel() const;

protected Q_SLOTS:
  /**
   * called when a header section is clicked in order to be sorted.
   * It results in that column being sorted if possible.
   */
  void onSortIndicatorChanged(int section, Qt::SortOrder order);

  /**
   * called when header data changes. We ensure that internal columns stay
   * hidden.
   */
  void onHeaderDataChanged();

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Overridden to tell the pqSpreadSheetViewModel about the active viewport.
   */
  void paintEvent(QPaintEvent* event) override;

private:
  Q_DISABLE_COPY(pqSpreadSheetViewWidget)

  class pqDelegate;

  bool SingleColumnMode;
  int OldColumnCount;
};

#endif
