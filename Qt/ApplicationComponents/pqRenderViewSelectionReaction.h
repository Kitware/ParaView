/*=========================================================================

   Program: ParaView
   Module:  pqRenderViewSelectionReaction.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#ifndef pqRenderViewSelectionReaction_h
#define pqRenderViewSelectionReaction_h

#include "pqSelectionReaction.h"
#include "vtkWeakPointer.h"
#include <QCursor>
#include <QPointer>
#include <QTimer>

class pqDataRepresentation;
class pqRenderView;
class pqView;
class vtkIntArray;
class vtkObject;
class vtkSMPVRepresentationProxy;

/**
* pqRenderViewSelectionReaction handles various selection modes available on
* RenderViews. Simply create multiple instances of
* pqRenderViewSelectionReaction to handle selection modes for that RenderView.
* pqRenderViewSelectionReaction uses internal static members to ensure that
* at most 1 view (and 1 type of selection) is in selection-mode at any given
* time.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqRenderViewSelectionReaction : public pqSelectionReaction
{
  Q_OBJECT
  typedef pqSelectionReaction Superclass;

public:
  enum SelectionMode
  {
    SELECT_SURFACE_CELLS,
    SELECT_SURFACE_POINTS,
    SELECT_FRUSTUM_CELLS,
    SELECT_FRUSTUM_POINTS,
    SELECT_SURFACE_CELLS_POLYGON,
    SELECT_SURFACE_POINTS_POLYGON,
    SELECT_BLOCKS,
    SELECT_CUSTOM_BOX,
    SELECT_CUSTOM_POLYGON,
    ZOOM_TO_BOX,
    CLEAR_SELECTION,
    GROW_SELECTION,
    SHRINK_SELECTION,
    SELECT_SURFACE_POINTDATA_INTERACTIVELY,
    SELECT_SURFACE_CELLDATA_INTERACTIVELY,
    SELECT_SURFACE_CELLS_INTERACTIVELY,
    SELECT_SURFACE_POINTS_INTERACTIVELY,
    SELECT_SURFACE_POINTS_TOOLTIP,
    SELECT_SURFACE_CELLS_TOOLTIP
  };

  /**
  * If \c view is NULL, this reaction will track the active-view maintained by
  * pqActiveObjects.
  */
  pqRenderViewSelectionReaction(QAction* parentAction, pqRenderView* view, SelectionMode mode,
    QActionGroup* modifierGroup = NULL);
  ~pqRenderViewSelectionReaction() override;

Q_SIGNALS:
  void selectedCustomBox(int xmin, int ymin, int xmax, int ymax);
  void selectedCustomBox(const int region[4]);
  void selectedCustomPolygon(vtkIntArray* polygon);

private Q_SLOTS:
  /**
  * For checkable actions, this calls this->beginSelection() or
  * this->endSelection() is val is true or false, respectively. For
  * non-checkable actions, this call this->beginSelection() and
  * this->endSelection() in that order.
  */
  virtual void actionTriggered(bool val);

  /**
   * Handles enable state for the `CLEAR_SELECTION`, `GROW_SELECTION`, and
   * `SHRINK_SELECTION` modes.
   */
  void updateEnableState() override;

  /**
  * Called when this object was created with NULL as the view and the active
  * view changes.
  */
  void setView(pqView* view);

  /**
  * Called when the active representation changes.
  */
  void setRepresentation(pqDataRepresentation* representation);

  /**
  * starts the selection i.e. setup render view in selection mode.
  */
  void beginSelection();

  /**
  * finishes the selection. Doesn't cause the selection, just returns the
  * render view to previous interaction mode.
  */
  void endSelection();

  /**
  * makes the pre-selection.
  */
  void preSelection();

  /**
  * makes fast pre-selection.
  */
  void fastPreSelection();

  /**
  * callback called for mouse stop events when in 'interactive selection'
  * modes.
  */
  void onMouseStop();

private:
  /**
  * callback called when the vtkPVRenderView is done with selection.
  */
  void selectionChanged(vtkObject*, unsigned long, void* calldata);

  /**
  * callback called for mouse move events when in 'interactive selection'
  * modes.
  */
  void onMouseMove();

  /**
  * callback called for click events when in 'interactive selection'
  * modes.
  */
  void onLeftButtonRelease();
  void onWheelRotate();

  // Get the current state of selection modifier
  int getSelectionModifier() override;

  // Check this selection is compatible with another type of selection
  bool isCompatible(SelectionMode mode);

  // Display/hide the tooltip of the selected point in mode SELECT_SURFACE_POINTS_TOOLTIP.
  void UpdateTooltip();

private:
  Q_DISABLE_COPY(pqRenderViewSelectionReaction)
  QPointer<pqRenderView> View;
  QPointer<pqDataRepresentation> Representation;
  QMetaObject::Connection RepresentationConnection;
  SelectionMode Mode;
  bool DisableSelectionModifiers;
  int PreviousRenderViewMode;
  vtkWeakPointer<vtkObject> ObservedObject;
  unsigned long ObserverIds[4];
  QCursor ZoomCursor;
  QTimer MouseMovingTimer;
  bool MouseMoving;
  int MousePosition[2];
  vtkSMPVRepresentationProxy* CurrentRepresentation = nullptr;

  static QPointer<pqRenderViewSelectionReaction> ActiveReaction;

  /**
  * cleans up observers.
  */
  void cleanupObservers();
};

#endif
