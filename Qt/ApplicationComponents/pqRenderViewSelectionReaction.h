// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRenderViewSelectionReaction_h
#define pqRenderViewSelectionReaction_h

#include "pqSelectionReaction.h"
#include "vtkWeakPointer.h"
#include <QCursor>
#include <QPointer>
#include <QShortcut>
#include <QString>
#include <QTimer>

class pqDataRepresentation;
class pqRenderView;
class pqView;
class vtkIntArray;
class vtkObject;
class vtkSMRepresentationProxy;

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
   * If \c view is nullptr, this reaction will track the active-view maintained by
   * pqActiveObjects.
   */
  pqRenderViewSelectionReaction(QAction* parentAction, pqRenderView* view, SelectionMode mode,
    QActionGroup* modifierGroup = nullptr);

  /**
   * Call CleanupObservers on destruction
   */
  ~pqRenderViewSelectionReaction() override;

Q_SIGNALS:

  ///@{
  /**
   * Signals emitted when the event happens
   */
  void selectedCustomBox(int xmin, int ymin, int xmax, int ymax);
  void selectedCustomBox(const int region[4]);
  void selectedCustomPolygon(vtkIntArray* polygon);
  ///@}

protected Q_SLOTS:
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
   * Called when this object was created with nullptr as the view and the active
   * view changes.
   * Please note that this method will cast the pqView to a pqRenderView.
   */
  virtual void setView(pqView* view);

  /**
   * Called when the active representation changes.
   */
  void setRepresentation(pqDataRepresentation* representation);

  /**
   * starts the selection i.e. setup render view in selection mode.
   */
  virtual void beginSelection();

  /**
   * finishes the selection. Doesn't cause the selection, just returns the
   * render view to previous interaction mode.
   */
  virtual void endSelection();

  /**
   * makes the pre-selection.
   */
  virtual void preSelection();

  /**
   * makes fast pre-selection.
   */
  virtual void fastPreSelection();

  /**
   * callback called for mouse stop events when in 'interactive selection'
   * modes.
   */
  virtual void onMouseStop();

  /**
   * clears the selection cache.
   */
  virtual void clearSelectionCache();

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * callback called when the vtkPVRenderView is done with selection.
   */
  virtual void selectionChanged(vtkObject*, unsigned long, void* calldata);

  /**
   * callback called for mouse move events when in 'interactive selection'
   * modes.
   */
  virtual void onMouseMove();

  ///@{
  /**
   * callback called for click events when in 'interactive selection' modes.
   */
  virtual void onLeftButtonRelease();
  virtual void onWheelRotate();
  virtual void onRightButtonPressed();
  virtual void onRightButtonRelease();
  ///@}

  /**
   * Get the current state of selection modifier
   */
  int getSelectionModifier() override;

  /**
   * Check this selection is compatible with another type of selection
   */
  virtual bool isCompatible(SelectionMode mode);

  /**
   *  Display/hide the tooltip of the selected point in mode SELECT_SURFACE_POINTS_TOOLTIP.
   */
  virtual void UpdateTooltip();

  /**
   * cleans up observers.
   */
  virtual void cleanupObservers();

private:
  Q_DISABLE_COPY(pqRenderViewSelectionReaction)
  QPointer<pqRenderView> View;
  QPointer<pqDataRepresentation> Representation;
  QMetaObject::Connection RepresentationConnection;
  SelectionMode Mode;
  int PreviousRenderViewMode;
  vtkWeakPointer<vtkObject> ObservedObject;
  unsigned long ObserverIds[6];
  QCursor ZoomCursor;
  QTimer MouseMovingTimer;
  bool MouseMoving;
  int MousePosition[2];
  bool DisablePreSelection = false;
  vtkSMRepresentationProxy* CurrentRepresentation = nullptr;
  QShortcut* CopyToolTipShortcut = nullptr;
  QString PlainTooltipText;

  static QPointer<pqRenderViewSelectionReaction> ActiveReaction;
};

#endif
