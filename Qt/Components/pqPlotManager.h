/*=========================================================================

   Program: ParaView
   Module:    pqPlotManager.h

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
#ifndef __pqGraphWindowManager_h
#define __pqGraphWindowManager_h

#include <QObject>
#include "pqComponentsExport.h"

class pqPlotManagerInternal;
class pqPlotViewModule;
class pqProxy;
class QWidget;

class PQCOMPONENTS_EXPORT pqPlotManager : public QObject
{
  Q_OBJECT
public:
  pqPlotManager(QObject* parent=0);
  virtual ~pqPlotManager();

signals:
  // Fired after a new plot module is noticed by the manager.
  void plotAdded(pqPlotViewModule*);

  // Fired just before the manager let's go of a plot view.
  void plotRemoved(pqPlotViewModule*);

  // Fired when the activeView changes.
  void activeViewChanged(pqPlotViewModule*);

public slots:
  void setActiveViewSilently(pqPlotViewModule*);
  void setActiveView(pqPlotViewModule* view);
  void renderAllViews();

protected:
  void onPlotAdded(pqPlotViewModule*);
  void onPlotRemoved(pqPlotViewModule*);

  // event filter callback.
  bool eventFilter(QObject* obj, QEvent* event);

  pqPlotViewModule* getViewModule(QWidget* widget);
private slots:
  void onProxyAdded(pqProxy* proxy);
  void onProxyRemoved(pqProxy* proxy);
  
private:
  pqPlotManager(const pqPlotManager&); // Not implemented.
  void operator=(const pqPlotManager&); // Not implemented.

  pqPlotManagerInternal* Internal;
};


#endif

