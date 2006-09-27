/*=========================================================================

   Program: ParaView
   Module:    pqRenderModule.h

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
#ifndef __pqRenderModule_h
#define __pqRenderModule_h


#include "pqGenericViewModule.h"

class pqRenderModuleInternal;
class pqUndoStack;
class QVTKWidget;
class vtkSMRenderModuleProxy;


// This is a PQ abstraction of a render module.
class PQCORE_EXPORT pqRenderModule : public pqGenericViewModule
{
  Q_OBJECT
public:
  typedef pqGenericViewModule Superclass;

  pqRenderModule(const QString& name, vtkSMRenderModuleProxy* renModule, 
    pqServer* server, QObject* parent=NULL);
  virtual ~pqRenderModule();

  /// Returns the internal render Module proxy associated with this object.
  vtkSMRenderModuleProxy* getRenderModuleProxy() const;

  /// Returns the QVTKWidget for this render Window.
  QVTKWidget* getWidget() const;

  /// Call this method to assign a Window in which this render module will
  /// render.  This will set the QVTKWidget's parent.
  virtual void setWindowParent(QWidget* parent);
  virtual QWidget* getWindowParent() const;

  /// Request a StillRender. 
  virtual void render();

  /// Resets the camera to include all visible data.
  void resetCamera();

  /// Save a screenshot for the render module. If width or height ==0,
  /// the current window size is used.
  virtual bool saveImage(int width, int height, const QString& filename);

  /// Each render module keeps a undo stack for interaction.
  /// This method returns that undo stack. External world
  /// typically uses it to Undo/Redo; pushing of elements on this stack
  /// on interaction is managed by this class.
  pqUndoStack* getInteractionUndoStack() const;

private slots:
  // Called on start/end interaction.
  void startInteraction();
  void endInteraction();

  // Called on start/end rendering.
  void onStartEvent();
  void onEndEvent();


protected:
  /// setups up RM and QVTKWidget binding.
  virtual void viewModuleInit();

private: 
  pqRenderModuleInternal* Internal;
};

#endif

