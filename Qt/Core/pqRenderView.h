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
  ~pqRenderView() override;

  /**
  * Returns the render view proxy associated with this object.
  */
  virtual vtkSMRenderViewProxy* getRenderViewProxy() const;

  /**
  * Resets the camera to include all visible data.
  * It is essential to call this resetCamera, to ensure that the reset camera
  * action gets pushed on the interaction undo stack.
  */
  void resetCamera() override;

  /**
  * Resets the center of rotation to the focal point.
  */
  virtual void resetCenterOfRotation();

  /**
  * Resets the parallel scale which is used for a parallel
  * projection
  */
  virtual void resetParallelScale();

  /**
  * Get if the orientation axes is visible.
  */
  virtual bool getOrientationAxesVisibility() const;

  /**
  * Get if the orientation axes is interactive.
  */
  virtual bool getOrientationAxesInteractivity() const;

  /**
  * Get orientation axes label color.
  */
  virtual QColor getOrientationAxesLabelColor() const;

  /**
  * Get orientation axes outline color.
  */
  virtual QColor getOrientationAxesOutlineColor() const;

  /**
  * Get whether resetCamera() resets the
  * center of rotation as well.
  */
  virtual bool getResetCenterWithCamera() const { return this->ResetCenterWithCamera; }

  /**
  * Get whether selection will be done on multiple representations.
  */
  virtual bool getUseMultipleRepresentationSelection() const
  {
    return this->UseMultipleRepresentationSelection;
  }

  /**
  * Get center axes visibility.
  */
  virtual bool getCenterAxesVisibility() const;

  /**
  * Get the current center of rotation.
  */
  virtual void getCenterOfRotation(double center[3]) const;

  /**
  * Returns if this view module can support
  * undo/redo. Returns false by default. Subclassess must override
  * if that's not the case.
  */
  bool supportsUndo() const override { return true; }

  /**
  * Returns if the view module can undo/redo interaction
  * given the current state of the interaction undo stack.
  */
  bool canUndo() const override;
  bool canRedo() const override;

  /**
  * Returns if this view module can support
  * image capture. Returns false by default. Subclassess must override
  * if that's not the case.
  */
  bool supportsCapture() const override { return true; }

  /**
  * For linking of interaction undo stacks.
  * This method is used by pqLinksModel to link
  * interaction undo stack for linked render views.
  */
  virtual void linkUndoStack(pqRenderView* other);
  virtual void unlinkUndoStack(pqRenderView* other);

  /**
  * Clears interaction undo stack of this view
  * (and all linked views, if any).
  */
  virtual void clearUndoStack();

  /**
  * Reset camera view direction
  */
  virtual void resetViewDirection(
    double look_x, double look_y, double look_z, double up_x, double up_y, double up_z);

  /**
  * Let internal class handle which internal widget should change its cursor
  * This is usually used for selection and in case of QuadView/SliceView
  * which contains an aggregation of QWidget, we don't necessary want all of
  * them to share the same cursor.
  */
  virtual void setCursor(const QCursor&);

  /**
  * Creates a new surface selection given the rectangle in display
  * coordinates.
  */
  virtual void selectOnSurface(
    int rectangle[4], int selectionModifier = pqView::PV_SELECTION_DEFAULT);
  virtual void selectPointsOnSurface(
    int rectangle[4], int selectionModifier = pqView::PV_SELECTION_DEFAULT);

  /**
  * Picks the representation at the given position.
  * This will result in firing the picked(pqOutputPort*) signal on successful
  * pick.
  */
  virtual pqDataRepresentation* pick(int pos[2]);

  /**
  * Picks the representation at the given position. Furthermore, if the
  * picked representation is a multi-block data set the picked block will
  * be returned in the flatIndex variable.
  */
  virtual pqDataRepresentation* pickBlock(int pos[2], unsigned int& flatIndex);

  /**
  * Creates a new frustum selection given the rectangle in display
  * coordinates.
  */
  virtual void selectFrustum(int rectangle[4]);
  virtual void selectFrustumPoints(int rectangle[4]);

  /**
  * Creates a "block" selection given the rectangle in display coordinates.
  * block selection is selection of a block in a composite dataset.
  */
  virtual void selectBlock(int rectangle[4], int selectionModifier = pqView::PV_SELECTION_DEFAULT);

  /**
  * Creates a new surface points selection given the polygon in display
  * coordinates.
  */
  virtual void selectPolygonPoints(
    vtkIntArray* polygon, int selectionModifier = pqView::PV_SELECTION_DEFAULT);

  /**
  * Creates a new surface cells selection given the polygon in display
  * coordinates.
  */
  virtual void selectPolygonCells(
    vtkIntArray* polygon, int selectionModifier = pqView::PV_SELECTION_DEFAULT);

signals:
  // Triggered when interaction mode change underneath
  void updateInteractionMode(int mode);

public slots:
  // Toggle the orientation axes visibility.
  virtual void setOrientationAxesVisibility(bool visible);

  // Toggle orientation axes interactivity.
  virtual void setOrientationAxesInteractivity(bool interactive);

  // Set orientation axes label color.
  virtual void setOrientationAxesLabelColor(const QColor&);

  // Set orientation axes outline color.
  virtual void setOrientationAxesOutlineColor(const QColor&);

  // Set the center of rotation. For this to work,
  // one should have appropriate interaction style (vtkPVInteractorStyle subclass)
  // and camera manipulators that use the center of rotation.
  // They are setup correctly by default.
  virtual void setCenterOfRotation(double x, double y, double z);
  virtual void setCenterOfRotation(double xyz[3])
  {
    this->setCenterOfRotation(xyz[0], xyz[1], xyz[2]);
  }

  // Set the parallel scale
  virtual void setParallelScale(double scale);

  // Toggle center axes visibility.
  virtual void setCenterAxesVisibility(bool visible);

  /**
  * Get/Set whether resetCamera() resets the
  * center of rotation as well.
  */
  virtual void setResetCenterWithCamera(bool b) { this->ResetCenterWithCamera = b; }

  /**
  * Set whether selection will be done on multiple representations.
  */
  virtual void setUseMultipleRepresentationSelection(bool b)
  {
    this->UseMultipleRepresentationSelection = b;
  }

  /**
  * start the link to other view process
  */
  virtual void linkToOtherView();

  /**
  * Called to undo interaction.
  * View modules supporting interaction undo must override this method.
  */
  virtual void undo() override;

  /**
  * Called to redo interaction.
  * View modules supporting interaction undo must override this method.
  */
  virtual void redo() override;

  /**
  * Resets center of rotation if this->ResetCenterWithCamera is true.
  */
  virtual void resetCenterOfRotationIfNeeded() { this->onResetCameraEvent(); }

  /**
  * Try to provide the best view orientation and interaction mode
  */
  virtual void updateInteractionMode(pqOutputPort* opPort);

protected slots:
  /**
  * Called when VTK event get trigger to notify that the interaction mode has changed
  */
  virtual void onInteractionModeChange();

  // Called when vtkSMRenderViewProxy fires
  // ResetCameraEvent.
  virtual void onResetCameraEvent();

  /**
  * Called when undo stack changes. We fires appropriate
  * undo signals as required by pqView.
  */
  virtual void onUndoStackChanged();

protected:
  /**
  * Updates undo stack without actually performing the undo/redo actions.
  */
  virtual void fakeUndoRedo(bool redo, bool self);

  /**
  * Updates undo stack of all linked views to record a programmatic change
  * in camera as a interaction. Must be called with start=true before the
  * change and with start=false after the change.
  */
  virtual void fakeInteraction(bool start);

  /**
  * Creates a new instance of the QWidget subclass to be used to show this
  * view. Default implementation creates a pqQVTKWidget
  */
  QWidget* createWidget() override;

  /**
  * Overridden to initialize the interaction undo/redo stack.
  */
  void initialize() override;

  // When true, the camera center of rotation will be reset when the
  // user reset the camera.
  bool ResetCenterWithCamera;

  // When true, the selection will be performed on all representations.
  bool UseMultipleRepresentationSelection;

private:
  void selectOnSurfaceInternal(int rect[4], QList<pqOutputPort*>&, bool select_points,
    int selectionModifier, bool select_blocks);

  void selectPolygonInternal(vtkIntArray* polygon, QList<pqOutputPort*>&, bool select_points,
    int selectionModifier, bool select_blocks);

  void emitSelectionSignal(QList<pqOutputPort*>);

  void collectSelectionPorts(vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources, QList<pqOutputPort*>& pqPorts, int selectionModifier,
    bool select_blocks);

  void InternalConstructor(vtkSMViewProxy* renModule);

  class pqInternal;
  pqInternal* Internal;
};

#endif
