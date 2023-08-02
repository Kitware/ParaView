// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpreadSheetView_h
#define pqSpreadSheetView_h

#include "pqView.h"

class vtkSMSourceProxy;
class pqDataRepresentation;
class pqSpreadSheetViewModel;

/**
 * View for spread-sheet view. It can show data from any source/filter on the
 * client. Uses pqSpreadSheetViewModel, pqSpreadSheetViewWidget and
 * pqSpreadSheetViewSelectionModel.
 */
class PQCORE_EXPORT pqSpreadSheetView : public pqView
{
  Q_OBJECT
  typedef pqView Superclass;

public:
  static QString spreadsheetViewType() { return "SpreadSheetView"; }

  pqSpreadSheetView(const QString& group, const QString& name, vtkSMViewProxy* viewModule,
    pqServer* server, QObject* parent = nullptr);
  ~pqSpreadSheetView() override;

  /**
   * Get the internal model for the view
   */
  pqSpreadSheetViewModel* getViewModel();

  /**
   * Returns the currently visible representation, if any.
   * Note that this view supports showing only one representation at a time.
   */
  pqDataRepresentation* activeRepresentation() const;

Q_SIGNALS:
  /**
   * Fired when the currently shown representation changes. \c repr may be
   * nullptr.
   */
  void showing(pqDataRepresentation* repr);
  void viewportUpdated();

public Q_SLOTS:
  /**
   * Called when a new repr is added.
   */
  void onAddRepresentation(pqRepresentation*);

protected Q_SLOTS:
  /**
   * Called to ensure that at most 1 repr is visible at a time.
   */
  void updateRepresentationVisibility(pqRepresentation* repr, bool visible);

  /**
   * Called at end of every render. We update the table view.
   */
  void onEndRender();

  /**
   * When user creates a "surface" selection on the view.
   */
  void onCreateSelection(vtkSMSourceProxy* selSource);

  /**
   * Called when checkbox "Show Only Selected Elements" is updated
   */
  void onSelectionOnly();

  /**
   * Called when the "Font Size" property is updated
   */
  void onFontSizeChanged();

  /**
   * Create a QWidget for the view's viewport.
   */
  QWidget* createWidget() override;

private:
  Q_DISABLE_COPY(pqSpreadSheetView)

  class pqInternal;
  pqInternal* Internal;
};

#endif
