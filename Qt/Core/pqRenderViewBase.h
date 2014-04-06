/*=========================================================================

   Program: ParaView
   Module:    pqRenderViewBase.h

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
#ifndef __pqRenderViewBase_h 
#define __pqRenderViewBase_h

#include "pqView.h"
#include "pqSMProxy.h" //needed for pqSMProxy.

class pqTimer;

/// pqRenderViewBase is an abstract base class for all render-view based views.
/// It encapuslates some of the commonly needed functionality for all such
/// views.
class PQCORE_EXPORT pqRenderViewBase : public pqView
{
  Q_OBJECT
  typedef pqView Superclass;

protected:
  // Constructor:
  // \c type  :- view type.
  // \c group :- SManager registration group name.
  // \c name  :- SManager registration name.
  // \c view  :- RenderView proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqRenderViewBase(const QString& type,
                   const QString& group,
                   const QString& name, 
                   vtkSMViewProxy* renModule, 
                   pqServer* server, 
                   QObject* parent=NULL);

public:
  // Destructor.
  virtual ~pqRenderViewBase();

  /// Returns the QVTKWidget for this render Window.
  virtual QWidget* getWidget();

  /// Resets the camera to include all visible data.
  /// It is essential to call this resetCamera, to ensure that the reset camera
  /// action gets pushed on the interaction undo stack.
  virtual void resetCamera()=0;

  /// Called to reset the view's display.  This method calls resetCamera().
  virtual void resetDisplay();

  /// Convenience method to enable stereo rendering on all views that support
  /// stereo rendering. If mode==0, stereo rendering is disabled. mode is same
  /// that used for vtkRenderWindow::SetStereoType.
  /// This does not request a render, the caller must explicitly call render on
  /// the views.
  static void setStereo(int mode);

protected slots:
  virtual void initializeAfterObjectsCreated();

  /// Setups up RenderModule and QVTKWidget binding.
  /// This method is called for all pqRenderView objects irrespective
  /// of whether it is created from state/undo-redo/python or by the GUI. Hence
  /// don't change any render module properties here.
  virtual void initializeWidgets()=0;

  /// Triggered by DelayNonInteractiveRenderEvent
  void beginDelayInteractiveRender();
  void endDelayInteractiveRender();

  /// Triggered by internal timer to update the status bar message
  void updateStatusMessage();

protected:
  /// Overridden to popup the context menu, if some actions have been added
  /// using addMenuAction.
  virtual bool eventFilter(QObject* caller, QEvent* e);

  /// Creates a new instance of the QWidget subclass to be used to show this
  /// view. Default implementation creates a QVTKWidget.
  virtual QWidget* createWidget();

  /// Use this method to initialize the pqObject state using the
  /// underlying vtkSMProxy. This needs to be done only once,
  /// after the object has been created. 
  virtual void initialize();

  /// On Mac, we usually try to cache the front buffer to avoid unecessary
  //  updates.
  bool AllowCaching;

private: 
  pqRenderViewBase(const pqRenderViewBase&); // Not implemented.
  void operator=(const pqRenderViewBase&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
  pqTimer* InteractiveDelayUpdateTimer;
};

#endif
