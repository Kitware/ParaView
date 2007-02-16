/*=========================================================================

   Program: ParaView
   Module:    pqRenderViewModule.h

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


#include "pqGenericViewModule.h"
#include <QColor> // needed for return type.

class pqRenderViewModuleInternal;
class QVTKWidget;
class QAction;
class vtkInteractorStyle;
class vtkSMRenderModuleProxy;
class vtkObject;

// This is a PQ abstraction of a render module.
class PQCORE_EXPORT pqRenderViewModule : public pqGenericViewModule
{
  Q_OBJECT
public:
  typedef pqGenericViewModule Superclass;

  static QString renderViewType() { return "RenderView"; }

  pqRenderViewModule(const QString& name, vtkSMRenderModuleProxy* renModule, 
    pqServer* server, QObject* parent=NULL);
  virtual ~pqRenderViewModule();

  /// Returns the internal render Module proxy associated with this object.
  vtkSMRenderModuleProxy* getRenderModuleProxy() const;

  /// Returns the QVTKWidget for this render Window.
  virtual QWidget* getWidget();

  /// Request a StillRender. 
  virtual void render();

  /// Resets the camera to include all visible data.
  void resetCamera();

  /// Resets the center of rotation to the focal point.
  void resetCenterOfRotation();

  /// Save a screenshot for the render module. If width or height ==0,
  /// the current window size is used.
  virtual bool saveImage(int width, int height, const QString& filename);

  /// Capture the view image into a new vtkImageData with the given magnification
  /// and returns it.
  virtual vtkImageData* captureImage(int magnification);

  /// Each render module keeps a undo stack for interaction.
  /// This method returns that undo stack. External world
  /// typically uses it to Undo/Redo; pushing of elements on this stack
  /// on interaction is managed by this class.
  virtual pqUndoStack* getInteractionUndoStack() const;

  /// Sets default values for the underlying proxy. This is typically called
  /// only on proxies created by the GUI itself.
  virtual void setDefaults();

  /// restore the default background color
  int* defaultBackgroundColor();
  
  /// restore the default light parameters
  void restoreDefaultLightSettings();

  /// Change the interactive style used by this render module. One is
  /// encouraged to use this method to change the interactive style
  /// rather than simply changing it on the render window interactor.
  void setInteractorStyle(vtkInteractorStyle* style);
 
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
  virtual bool canDisplaySource(pqPipelineSource* source) const;

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

  // Initializes the interactor style and center axes using server state.
  // This is called after a state file is finished loading.
  void updateInteractorStyleFromState();

  // Toggle center axes visibility.
  void setCenterAxesVisibility(bool visible);

  /// Get/Set whether resetCamera() resets the
  /// center of rotation as well.
  void setResetCenterWithCamera(bool b)
    { this->ResetCenterWithCamera = b;}

  /// start the link to other view process
  void linkToOtherView();

private slots:
  // Called on start/end interaction.
  void startInteraction();
  void endInteraction();

  // Called when vtkSMRenderModuleProxy fires
  // ResetCameraEvent.
  void onResetCameraEvent();

protected:
  /// setups up RM and QVTKWidget binding.
  virtual void viewModuleInit();

  bool eventFilter(QObject* caller, QEvent* e);

  // When true, the camera center of rotation will be reset when the
  // user reset the camera.
  bool ResetCenterWithCamera;

private: 
  pqRenderViewModuleInternal* Internal;
};

#endif

