/*=========================================================================

   Program: ParaView
   Module:    pqRenderView.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#ifndef __pqRenderViewModule_h
#define __pqRenderViewModule_h


#include "pqView.h"
#include <QColor> // needed for return type.

class QAction;
class QVTKWidget;
class vtkSMRenderViewProxy;

// This is a PQ abstraction of a render view.
class PQCORE_EXPORT pqRenderView : public pqView
{
  Q_OBJECT
  typedef pqView Superclass;
public:
  static QString renderViewType() { return "RenderView"; }
  static QString renderViewTypeName() { return "3D View"; }

  // Constructor:
  // \c group :- SManager registration group name.
  // \c name  :- SManager registration name.
  // \c view  :- RenderView proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqRenderView( const QString& group,
                const QString& name, 
                vtkSMViewProxy* renModule, 
                pqServer* server, 
                QObject* parent=NULL);

  // Destructor.
  virtual ~pqRenderView();

  /// Returns the render view proxy associated with this object.
  virtual vtkSMRenderViewProxy* getRenderViewProxy() const;

  /// Returns the QVTKWidget for this render Window.
  virtual QWidget* getWidget();

  /// Request a StillRender. 
  virtual void render();

  /// Resets the camera to include all visible data.
  /// It is essential to call this resetCamera, to ensure that the reset camera
  /// action gets pushed on the interaction undo stack.
  void resetCamera();

  /// Resets the center of rotation to the focal point.
  void resetCenterOfRotation();

  /// Save a screenshot for the render module. If width or height ==0,
  /// the current window size is used.
  virtual bool saveImage(int width, int height, const QString& filename);

  /// Capture the view image into a new vtkImageData with the given magnification
  /// and returns it.
  virtual vtkImageData* captureImage(int magnification);

  /// Sets default values for the underlying proxy. 
  /// This is during the initialization stage of the pqProxy 
  /// for proxies created by the GUI itself i.e.
  /// for proxies loaded through state or created by python client
  /// this method won't be called. 
  virtual void setDefaultPropertyValues();

  /// restore the default background color
  int* defaultBackgroundColor();
  
  /// restore the default light parameters
  void restoreDefaultLightSettings();

  /// Save the settings of this render module with QSettings
  virtual void saveSettings();

  /// Apply the settings from QSettings to this render module
  virtual void restoreSettings();

  /// Get if the orientation axes is visible.
  bool getOrientationAxesVisibility() const;

  /// Get if the orientation axes is interactive.
  bool getOrientationAxesInteractivity() const;

  /// Get orientation axes label color.
  QColor getOrientationAxesLabelColor() const;

  /// Get orientation axes outline color.
  QColor getOrientationAxesOutlineColor() const;

  /// Get whether resetCamera() resets the
  /// center of rotation as well.
  bool getResetCenterWithCamera() const
    { return this->ResetCenterWithCamera; }

  /// Get center axes visibility.
  bool getCenterAxesVisibility() const;

  /// Get the current center of rotation.
  void getCenterOfRotation(double center[3]) const;

  /// add an action for a context menu
  void addMenuAction(QAction* a);
  /// remove an action for a context menu
  void removeMenuAction(QAction* a);

  /// Returns if this view module can support 
  /// undo/redo. Returns false by default. Subclassess must override
  /// if that's not the case.
  virtual bool supportsUndo() const { return true; }
 
  // returns whether a source can be displayed in this view module 
  virtual bool canDisplay(pqOutputPort* opPort) const;

  /// Returns if the view module can undo/redo interaction
  /// given the current state of the interaction undo stack.
  bool canUndo() const;
  bool canRedo() const;

  /// For linking of interaction undo stacks.
  /// This method is used by pqLinksModel to link 
  /// interaction undo stack for linked render views.
  void linkUndoStack(pqRenderView* other);
  void unlinkUndoStack(pqRenderView* other);

  /// Clears interaction undo stack of this view
  /// (and all linked views, if any).
  void clearUndoStack();

  /// Get/set the camera manipulators
  QList<vtkSMProxy*> getCameraManipulators() const;
  virtual bool updateDefaultInteractors(QList<vtkSMProxy*> manipulators);

  /// Create a CameraManipulatorProxy given the mouse, key and name.
  /// Whoever calling this is reponsible for deleting the new proxy.
  virtual vtkSMProxy* createCameraManipulator(
    int mouse, int shift, int control, QString name);

  /// Restore the default camera manipulators
  QList<vtkSMProxy*> getDefaultCameraManipulators() const;

  /// Reset camera view direction
  void resetViewDirection(
    double look_x, double look_y, double look_z,
    double up_x, double up_y, double up_z);

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
  void setCenterOfRotation(double xyz[3])
    {
    this->setCenterOfRotation(xyz[0], xyz[1], xyz[2]);
    }

  // Sets the position and scale of the axes when the center of rotation has been modified
  void updateCenterAxes();

  // Toggle center axes visibility.
  void setCenterAxesVisibility(bool visible);

  /// Get/Set whether resetCamera() resets the
  /// center of rotation as well.
  void setResetCenterWithCamera(bool b)
    { this->ResetCenterWithCamera = b;}

  /// start the link to other view process
  void linkToOtherView();

  /// Called to undo interaction.
  /// View modules supporting interaction undo must override this method.
  virtual void undo();

  /// Called to redo interaction.
  /// View modules supporting interaction undo must override this method.
  virtual void redo();

  /// Creates a new surface selection given the rectangle in display
  /// coordinates.
  void selectOnSurface(int rectangle[4]);
  void selectPointsOnSurface(int rectangle[4]);

  /// Creates a new frustum selection given the rectangle in display
  /// coordinates.
  void selectFrustum(int rectangle[4]);
  void selectFrustumPoints(int rectangle[4]);

private slots:
  // Called when vtkSMRenderViewProxy fires
  // ResetCameraEvent.
  void onResetCameraEvent();

  /// Setups up RenderModule and QVTKWidget binding.
  /// This method is called for all pqRenderView objects irrespective
  /// of whether it is created from state/undo-redo/python or by the GUI. Hence
  /// don't change any render module properties here.
  void initializeWidgets();

  /// Called when undo stack changes. We fires appropriate 
  /// undo signals as required by pqView.
  void onUndoStackChanged();

protected:
  bool eventFilter(QObject* caller, QEvent* e);

  /// Creates default interactor style/manipulators.
  void createDefaultInteractors();

  /// Center Axes represents the 3D axes in the view. When GUI creates the view,
  /// we explicitly create a center axes, register it and add it to the view 
  /// displays.
  /// However, when the view is not created by GUI explicitly i.e. created 
  /// through undo-redo/state/python, we try to use the first Axes display in 
  /// the view as the center axes if any. Otherwise a new center axes will be 
  /// created for the view then setCenterAxesVisibility(true) is called. Thus, 
  /// for such views the behaviour is analogous to center axis visibility being 
  /// off. Once, the user enables the center axes, we will show one.
  void initializeCenterAxes();

  // When true, the camera center of rotation will be reset when the
  // user reset the camera.
  bool ResetCenterWithCamera;

  /// Updates undo stack without actually performing the undo/redo actions.
  void fakeUndoRedo(bool redo, bool self);

  /// Updates undo stack of all linked views to record a programatic change
  /// in camera as a interaction. Must be called with start=true before the
  /// change and with start=false after the change.
  void fakeInteraction(bool start);

  /// Creates a new instance of the QWidget subclass to be used to show this
  /// view. Default implementation creates a QVTKWidget.
  virtual QWidget* createWidget();

  /// Use this method to initialize the pqObject state using the
  /// underlying vtkSMProxy. This needs to be done only once,
  /// after the object has been created. 
  virtual void initialize();
private: 
  class pqInternal;
  pqInternal* Internal;
};

#endif

