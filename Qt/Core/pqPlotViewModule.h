/*=========================================================================

   Program: ParaView
   Module:    pqPlotViewModule.h

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
#ifndef __pqPlotViewModule_h
#define __pqPlotViewModule_h

#include "pqGenericViewModule.h"

class pqPlotViewModuleInternal;

class PQCORE_EXPORT pqPlotViewModule : public pqGenericViewModule
{
  Q_OBJECT
public:
  typedef pqGenericViewModule Superclass;

  pqPlotViewModule(int type, const QString& group, const QString& name, 
    vtkSMAbstractViewModuleProxy* renModule, 
    pqServer* server, QObject* parent=NULL);
  virtual ~pqPlotViewModule();

  enum PlotType
    {
    BAR_CHART = 0,
    XY_PLOT =1
    };

  QWidget* getWidget();

  /// Call this method to assign a Window in which this view module will
  /// be displayed.
  virtual void setWindowParent(QWidget* parent);
  virtual QWidget* getWindowParent() const;

  /// Save a screenshot for the render module. If width or height ==0,
  /// the current window size is used.
  virtual bool saveImage(int /*width*/, int /*height*/, 
    const QString& /*filename*/) 
    {
    // Not supported yet.
    return false;
    };

  int getType()
    { return this->Type; }
   
  /// Forces an immediate render. Overridden since for plots
  /// rendering actually happens on the GUI side, not merely
  /// in the ServerManager.
  virtual void forceRender();

private slots:
  void visibilityChanged(pqDisplay* disp);

protected:
  int Type;
  void renderBarChart();
  void renderXYPlot();

private:
  pqPlotViewModule(const pqPlotViewModule&); // Not implemented.
  void operator=(const pqPlotViewModule&); // Not implemented.

  pqPlotViewModuleInternal* Internal;
};


#endif

