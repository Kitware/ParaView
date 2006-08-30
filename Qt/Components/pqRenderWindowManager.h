/*=========================================================================

   Program: ParaView
   Module:    pqRenderWindowManager.h

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
#ifndef __pqRenderWindowManager_h
#define __pqRenderWindowManager_h


#include "pqComponentsExport.h"
#include "pqMultiView.h"

class pqMultiView;
class pqMultiViewFrame;
class pqRenderModule;
class pqRenderWindowManagerInternal;
class pqServer;
class vtkPVXMLElement;
class vtkSMStateLoader;

// This class manages all render windows. This class is an attempt to
// take away some of the work form pqMainWindow. It extends pqMultiView
// to add support to add render modules in multiview.
class PQCOMPONENTS_EXPORT pqRenderWindowManager : public pqMultiView 
{
  Q_OBJECT
public:
  pqRenderWindowManager(QWidget* parent=NULL);
  virtual ~pqRenderWindowManager();

  // returns the active render module.
  pqRenderModule* getActiveRenderModule();

  // Save the state of the render window manager.
  void saveState(vtkPVXMLElement* root);

  // Loads the state for the render window manager.
  bool loadState(vtkPVXMLElement* rwRoot, vtkSMStateLoader* loader);
signals:
  // Fired when the active render module changes.
  void activeRenderModuleChanged(pqRenderModule*);

private slots:
  /// This will create a RenderWindow to fill the frame.
  /// the render window is created on the active server
  /// which must be set by the application.
  void onFrameAdded(pqMultiViewFrame* frame);
  void onFrameRemoved(pqMultiViewFrame* frame);

  void onRenderModuleAdded(pqRenderModule* rm);
  void onRenderModuleRemoved(pqRenderModule* rm);

  void onActivate(QWidget* obj);

  void frameDragStart(pqMultiViewFrame*);
  void frameDragEnter(pqMultiViewFrame*,QDragEnterEvent*);
  void frameDragMove(pqMultiViewFrame*,QDragMoveEvent*);
  void frameDrop(pqMultiViewFrame*,QDropEvent*);

public slots:
  void setActiveServer(pqServer* server);
  void allocateWindowsToRenderModules();

protected:
  // Event filter callback.
  bool eventFilter(QObject* caller, QEvent* e);

  virtual void loadState(vtkPVXMLElement* root)
    {
    this->pqMultiView::loadState(root);
    }
private:
  pqRenderWindowManagerInternal* Internal;
};

#endif

