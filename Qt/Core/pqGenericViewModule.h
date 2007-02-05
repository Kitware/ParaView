/*=========================================================================

   Program: ParaView
   Module:    pqGenericViewModule.h

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
#ifndef __pqGenericViewModule_h
#define __pqGenericViewModule_h


#include "pqProxy.h"

class pqDisplay;
class pqGenericViewModuleInternal;
class pqPipelineSource;
class pqServer;
class pqUndoStack;
class QWidget;
class vtkImageData;
class vtkSMAbstractViewModuleProxy;


/// This is a PQ abstraction of a generic view module. Subclasses can be
/// specific for different types of view such as render view, histogram view
/// etc.
class PQCORE_EXPORT pqGenericViewModule : public pqProxy
{
  Q_OBJECT
public:
  pqGenericViewModule(const QString& group, const QString& name, 
    vtkSMAbstractViewModuleProxy* renModule, 
    pqServer* server, QObject* parent=NULL);
  virtual ~pqGenericViewModule();

  /// Returns the internal render Module proxy associated with this object.
  vtkSMAbstractViewModuleProxy* getViewModuleProxy() const;

  /// Return a widget associated with this view
  virtual QWidget* getWidget() = 0;

  /// Call this method to assign a Window in which this view module will
  /// be displayed.
  virtual void setWindowParent(QWidget* parent)=0;
  virtual QWidget* getWindowParent() const =0;

  /// Returns if this view module can support 
  /// undo/redo. Returns false by default. Subclassess must override
  /// if that's not the case.
  virtual bool supportsUndo() const { return false; }

  /// View modules that support undo must override this method
  /// to return the undo stack for the view module.
  virtual pqUndoStack* getInteractionUndoStack() const { return 0;} 

public slots:
  /// Request a StillRender. Default implementation simply calls
  /// forceRender(). Subclasses can implement a delayed/buffered render.
  virtual void render() { this->forceRender(); }

  /// Forces an immediate render.
  virtual void forceRender();

public:

  /// Save a screenshot for the render module. If width or height ==0,
  /// the current window size is used.
  virtual bool saveImage(int width, int height, const QString& filename) =0;

  /// Capture the view image into a new vtkImageData with the given magnification
  /// and returns it.
  virtual vtkImageData* captureImage(int magnification) =0;

  /// This method checks if the display is one of the displays
  /// rendered by this render module.
  bool hasDisplay(pqDisplay* display);

  /// Gets the number of displays in the render module.
  int getDisplayCount() const;

  /// Gets the display for the specified index.
  pqDisplay* getDisplay(int index) const;
 
  /// Returns a list of displays in this render module.
  QList<pqDisplay*> getDisplays() const;

  /// This method returns is any pqPipelineSource can be dislayed in this
  /// view. This is a convenience method, it gets
  /// the pqDisplayPolicy object from the pqApplicationCore
  /// are queries it.
  bool canDisplaySource(pqPipelineSource* source) const;

signals:
  /// Fired after a display has been added to this render module.
  void displayAdded(pqDisplay*);

  /// Fired after a display has been removed from this render module.
  void displayRemoved(pqDisplay*);

  /// Fired when the render module fires a vtkCommand::StartEvent
  /// signalling the beginning of rendering. Subclasses must fire
  /// these signals at appropriate times.
  void beginRender();

  /// Fired when the render module fires a vtkCommand::EndEvent
  /// signalling the end of rendering.
  /// Subclasses must fire these signals at appropriate times.
  void endRender();

  /// Fired when any displays visibility changes.
  void displayVisibilityChanged(pqDisplay* display, bool visible);

private slots:
  /// if renModule is not created when this object is instantianted, we
  /// must listen to UpdateVTKObjects event to bind the QVTKWidget and
  /// then render window.
  void onUpdateVTKObjects();

  /// Called when the "Displays" property changes.
  void displaysChanged();

  void onDisplayVisibilityChanged(bool);

protected:
  /// Called to initialize the view module either when this object is created
  /// on when the proxy is created.
  virtual void viewModuleInit();

private:
  pqGenericViewModule(const pqGenericViewModule&); // Not implemented.
  void operator=(const pqGenericViewModule&); // Not implemented.

  pqGenericViewModuleInternal* Internal;
};

#endif

