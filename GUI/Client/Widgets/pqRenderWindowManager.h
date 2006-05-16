/*=========================================================================

   Program:   ParaQ
   Module:    pqRenderWindowManager.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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


#include "pqWidgetsExport.h"
#include <QObject>

class pqServer;
class pqMultiViewFrame;
class pqRenderModule;
class pqRenderWindowManagerInternal;

// This class manages all render windows. This class is an attempt to
// take away some of the work form pqMainWindow. 
class PQWIDGETS_EXPORT pqRenderWindowManager : public QObject 
{
  Q_OBJECT
public:
  pqRenderWindowManager(QObject* parent=NULL);
  virtual ~pqRenderWindowManager();

  // returns the active render module.
  pqRenderModule* getActiveRenderModule();

public slots:
  // this must be set, so that the manager knows on which server
  // to create the view when a new view is added.
  void setActiveServer(pqServer* server);

public slots:
  /// This will create a RenderWindow to fill the frame.
  /// the render window is created on the active server
  /// which must be set by the application.
  void onFrameAdded(pqMultiViewFrame* frame);
  void onFrameRemoved(pqMultiViewFrame* frame);

  void onRenderModuleAdded(pqRenderModule* rm);
  void onRenderModuleRemoved(pqRenderModule* rm);

  void onActivate(QWidget* obj);
private:
  pqRenderWindowManagerInternal* Internal;
};

#endif

