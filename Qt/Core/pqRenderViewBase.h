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
#ifndef pqRenderViewBase_h
#define pqRenderViewBase_h

#include "pqSMProxy.h" //needed for pqSMProxy.
#include "pqView.h"
class pqTimer;

/**
* pqRenderViewBase is an abstract base class for all render-view based views.
* It encapuslates some of the commonly needed functionality for all such
* views.
*/
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
  pqRenderViewBase(const QString& type, const QString& group, const QString& name,
    vtkSMViewProxy* renModule, pqServer* server, QObject* parent = NULL);

public:
  // Destructor.
  ~pqRenderViewBase() override;

  /**
  * Resets the camera to include all visible data.
  * It is essential to call this resetCamera, to ensure that the reset camera
  * action gets pushed on the interaction undo stack.
  */
  virtual void resetCamera() = 0;

  /**
  * Called to reset the view's display.  This method calls resetCamera().
  */
  void resetDisplay() override;

protected Q_SLOTS:
  virtual void initializeAfterObjectsCreated();

  /**
  * Triggered by DelayNonInteractiveRenderEvent
  */
  void beginDelayInteractiveRender();
  void endDelayInteractiveRender();

  /**
  * Triggered by internal timer to update the status bar message
  */
  void updateStatusMessage();

protected:
  /**
  * Overridden to popup the context menu, if some actions have been added
  * using addMenuAction.
  */
  bool eventFilter(QObject* caller, QEvent* e) override;

  /**
  * Creates a new instance of the QWidget subclass to be used to show this
  * view. Default implementation creates a pqQVTKWidget.
  */
  QWidget* createWidget() override;

  /**
  * Use this method to initialize the pqObject state using the
  * underlying vtkSMProxy. This needs to be done only once,
  * after the object has been created.
  */
  void initialize() override;

private:
  Q_DISABLE_COPY(pqRenderViewBase)

  class pqInternal;
  pqInternal* Internal;
  pqTimer* InteractiveDelayUpdateTimer;
};

#endif
