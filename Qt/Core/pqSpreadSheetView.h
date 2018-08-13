/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetView.h

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

public:
  pqSpreadSheetView(const QString& group, const QString& name, vtkSMViewProxy* viewModule,
    pqServer* server, QObject* parent = NULL);
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

signals:
  /**
  * Fired when the currently shown representation changes. \c repr may be
  * NULL.
  */
  void showing(pqDataRepresentation* repr);
  void viewportUpdated();

public slots:
  /**
  * Called when a new repr is added.
  */
  void onAddRepresentation(pqRepresentation*);

protected slots:
  /**
  * Called to ensure that at most 1 repr is visible at a time.
  */
  void updateRepresentationVisibility(pqRepresentation* repr, bool visible);

  /**
  * Called at start of every render.
  * If in "selection-only" mode, and showing composite dataset, we want to make
  * sure that we are shown a block with non-empty cells/points (if possible).
  */
  void onBeginRender();

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
