/*=========================================================================

   Program: ParaView
   Module:    pqRenderView.h

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
#ifndef pqRenderViewModule_h
#define pqRenderViewModule_h

#include "pqRenderViewBase.h"
#include <QColor> // needed for return type.

class pqDataRepresentation;
class QAction;
class vtkCollection;
class vtkIntArray;
class vtkSMRenderViewProxy;

// This is a PQ abstraction of a render view.
class PQCORE_EXPORT pqRenderView : public pqRenderViewBase
{
  Q_OBJECT
  typedef pqRenderViewBase Superclass;

public:
  static QString renderViewType() { return "RenderView"; }

  // Constructor:
  // \c group :- SManager registration group name.
  // \c name  :- SManager registration name.
  // \c view  :- RenderView proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqRenderView(const QString& group, const QString& name, vtkSMViewProxy* renModule,
    pqServer* server, QObject* parent = NULL);

  // This version allows subclasses to substitute their own renderViewType.
  pqRenderView(const QString& tname, const QString& group, const QString& name,
    vtkSMViewProxy* renModule, pqServer* server, QObject* parent = NULL);

  // Destructor.
  virtual ~pqRenderView();

  /**
  * Returns the render view proxy associated with this object.
  */
  virtual vtkSMRenderViewProxy* getRenderViewProxy() const;

  /**
  * Resets the camera to include all visible data.
  * It is essential to call this resetCamera, to ensure that the reset camera
  * action gets pushed on the interaction undo stack.
  */
  virtual void resetCamera();

  /**
  * Resets the center of rotation to the focal point.
  */
  void resetCenterOfRotation();

  /**
  * Get if the orientation axes is visible.
  */
  bool getOrientationAxesVisibility() const;

  /**
  * Get if the orientation axes is interactive.
  */
  bool getOrientationAxesInteractivity() const;

  /**
  * Get orientation axes label color.
  */
  QColor getOrientationAxesLabelColor() const;

  /**
  * Get orientation axes outline color.
  */
  QColor getOrientationAxesOutlineColor() const;

  /**
  * Get whether resetCamera() resets the
  * center of rotation as well.
  */
  bool getResetCenterWithCamera() const { return this->ResetCenterWithCamera; }

  /**
  * Get whether selection will be done on multiple representations.
  */
  bool getUseMultipleRepresentationSelection() const
  {
    return this->UseMultipleRepresentationSelection;
  }

  /**
  * Get center axes visibility.
  */
  bool getCenterAxesVisibility() const;

  /**
  * Get the current center of rotation.
  */
  void getCenterOfRotation(double center[3]) const;

  /**
  * Returns if this view module can support
  * undo/redo. Returns false by default. Subclassess must override
  * if that's not the case.
  */
  virtual bool supportsUndo() const { return true; }

  /**
  * Returns if the view module can undo/redo interaction
  * given the current state of the interaction undo stack.
  */
  virtual bool canUndo() const;
  virtual bool canRedo() const;

  /**
  * For linking of interaction undo stacks.
  * This method is used by pqLinksModel to link
  * interaction undo stack for linked render views.
  */
  void linkUndoStack(pqRenderView* other);
  void unlinkUndoStack(pqRenderView* other);

  /**
  * Clears interaction undo stack of this view
  * (and all linked views, if any).
  */
  void clearUndoStack();

  /**
  * Reset camera view direction
  */
  void resetViewDirection(
    double look_x, double look_y, double look_z, double up_x, double up_y, double up_z);

  /**
  * Let internal class handle which internal widget should change its cursor
  * This is usually used for selection and in case of QuadView/SliceView
  * which contains an aggregation of QWidget, we don't necessary want all of
  * them to share the same cursor.
  */
  virtual void setCursor(const QCursor&);

public:
  /**
  * Creates a new surface selection given the rectangle in display
  * coordinates.
  */
  void selectOnSurface(int rectangle[4], int selectionModifier = pqView::PV_SELECTION_DEFAULT);
  void selectPointsOnSurface(
    int rectangle[4], int selectionModifier = pqView::PV_SELECTION_DEFAULT);

  /**
  * Picks the representation at the given position.
  * This will result in firing the picked(pqOutputPort*) signal on successful
  * pick.
  */
  pqDataRepresentation* pick(int pos[2]);

  /**
  * Picks the representation at the given position. Furthermore, if the
  * picked representation is a multi-block data set the picked block will
  * be returned in the flatIndex variable.
  */
  pqDataRepresentation* pickBlock(int pos[2], unsigned int& flatIndex);

  /**
  * Creates a new frustum selection given the rectangle in display
  * coordinates.
  */
  void selectFrustum(int rectangle[4]);
  void selectFrustumPoints(int rectangle[4]);

  /**
  * Creates a "block" selection given the rectangle in display coordinates.
  * block selection is selection of a block in a composite dataset.
  */
  void selectBlock(int rectangle[4], int selectionModifier = pqView::PV_SELECTION_DEFAULT);

  /**
  * Creates a new surface points selection given the polygon in display
  * coordinates.
  */
  void selectPolygonPoints(
    vtkIntArray* polygon, int selectionModifier = pqView::PV_SELECTION_DEFAULT);

  /**
  * Creates a new surface cells selection given the polygon in display
  * coordinates.
  */
  void selectPolygonCells(
    vtkIntArray* polygon, int selectionModifier = pqView::PV_SELECTION_DEFAULT);

signals:
  // Triggered when interaction mode change underneath
  void updateInteractionMode(int mode);

public slots:
  // Toggle the orientation axes visibility.
  void setOrientationAxesVisibility(bool visible);

  // Toggle orientation axes interactivity.
  void setOrientationAxesInteractivity(bool interactive);

  // Set orientation axes label color.
  void setOrientationAxesLabelColor(const QColor&);

  // Set orientation axes outline color.
  void setOrientationAxesOutlineColor(const QColor&);

  // Set the center of rotation. For this to work,
  // one should have approriate interaction style (vtkPVInteractorStyle subclass)
  // and camera manipulators that use the center of rotation.
  // They are setup correctly by default.
  void setCenterOfRotation(double x, double y, double z);
  void setCenterOfRotation(double xyz[3]) { this->setCenterOfRotation(xyz[0], xyz[1], xyz[2]); }

  // Toggle center axes visibility.
  void setCenterAxesVisibility(bool visible);

  /**
  * Get/Set whether resetCamera() resets the
  * center of rotation as well.
  */
  void setResetCenterWithCamera(bool b) { this->ResetCenterWithCamera = b; }

  /**
  * Set whether selection will be done on multiple representations.
  */
  void setUseMultipleRepresentationSelection(bool b)
  {
    this->UseMultipleRepresentationSelection = b;
  }

  /**
  * start the link to other view process
  */
  void linkToOtherView();

  /**
  * Called to undo interaction.
  * View modules supporting interaction undo must override this method.
  */
  virtual void undo();

  /**
  * Called to redo interaction.
  * View modules supporting interaction undo must override this method.
  */
  virtual void redo();

  /**
  * Resets center of rotation if this->ResetCenterWithCamera is true.
  */
  void resetCenterOfRotationIfNeeded() { this->onResetCameraEvent(); }

  /**
  * Try to provide the best view orientation and interaction mode
  */
  void updateInteractionMode(pqOutputPort* opPort);

private slots:
  // Called when vtkSMRenderViewProxy fires
  // ResetCameraEvent.
  void onResetCameraEvent();

  /**
  * Called when undo stack changes. We fires appropriate
  * undo signals as required by pqView.
  */
  void onUndoStackChanged();

  /**
  * Called when VTK event get trigger to notify that the interaction mode has changed
  */
  void onInteractionModeChange();

protected:
  // When true, the camera center of rotation will be reset when the
  // user reset the camera.
  bool ResetCenterWithCamera;

  // When true, the selection will be performed on all representations.
  bool UseMultipleRepresentationSelection;

  /**
  * Updates undo stack without actually performing the undo/redo actions.
  */
  void fakeUndoRedo(bool redo, bool self);

  /**
  * Updates undo stack of all linked views to record a programatic change
  * in camera as a interaction. Must be called with start=true before the
  * change and with start=false after the change.
  */
  void fakeInteraction(bool start);

  /**
  * Creates a new instance of the QWidget subclass to be used to show this
  * view. Default implementation creates a pqQVTKWidget
  */
  virtual QWidget* createWidget();

  /**
  * Overridden to initialize the interaction undo/redo stack.
  */
  virtual void initialize();

private:
  class pqInternal;
  pqInternal* Internal;
  void selectOnSurfaceInternal(int rect[4], QList<pqOutputPort*>&, bool select_points,
    int selectionModifier, bool select_blocks);
  void selectPolygonInternal(vtkIntArray* polygon, QList<pqOutputPort*>&, bool select_points,
    int selectionModifier, bool select_blocks);

  void emitSelectionSignal(QList<pqOutputPort*>);
  void collectSelectionPorts(vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources, QList<pqOutputPort*>& pqPorts, int selectionModifier,
    bool select_blocks);

  void InternalConstructor(vtkSMViewProxy* renModule);
};

#endif
