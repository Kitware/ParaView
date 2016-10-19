/*=========================================================================

   Program: ParaView
   Module:    pqTraceReaction.cxx

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
#include "pqTraceReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqPVApplicationCore.h"
#include "pqProxyWidgetDialog.h"
#include "pqServer.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include "vtkPVConfig.h"
#ifdef PARAVIEW_ENABLE_PYTHON
#include "pqPythonManager.h"
#endif
//-----------------------------------------------------------------------------
pqTraceReaction::pqTraceReaction(
  QAction* parentObject, const char* start_trace_label, const char* stop_trace_label)
  : Superclass(parentObject)
  , StartTraceLabel(start_trace_label)
  , StopTraceLabel(stop_trace_label)
{
#ifdef PARAVIEW_ENABLE_PYTHON
  this->parentAction()->setEnabled(true);
  this->parentAction()->setText(
    vtkSMTrace::GetActiveTracer() == NULL ? this->StartTraceLabel : this->StopTraceLabel);
#else
  this->parentAction()->setEnabled(false);
  this->parentAction()->setToolTip(
    "Tracing unavailable since application built without Python support.");
  this->parentAction()->setStatusTip(
    "Tracing unavailable since application built without Python support.");
#endif
}

//-----------------------------------------------------------------------------
pqTraceReaction::~pqTraceReaction()
{
  // ensure to stop trace before the application quits
  if (vtkSMTrace::GetActiveTracer())
  {
    vtkSMTrace::StopTrace();
  }
}

//-----------------------------------------------------------------------------
void pqTraceReaction::onTriggered()
{
  if (vtkSMTrace::GetActiveTracer() == NULL)
  {
    this->start();
  }
  else
  {
    this->stop();
  }
  this->parentAction()->setText(
    vtkSMTrace::GetActiveTracer() == NULL ? this->StartTraceLabel : this->StopTraceLabel);
}

//-----------------------------------------------------------------------------
void pqTraceReaction::start()
{
  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().activeServer()->proxyManager();

  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("pythontracing", "PythonTraceOptions"));
  if (proxy)
  {
    vtkNew<vtkSMParaViewPipelineController> controller;
    controller->InitializeProxy(proxy);
    pqProxyWidgetDialog dialog(proxy);
    dialog.setWindowTitle("Trace Options");
    dialog.setObjectName("TraceOptionsDialog");
    dialog.setApplyChangesImmediately(true);
    if (dialog.exec() != QDialog::Accepted)
    {
      return;
    }
  }
  vtkSMTrace* trace = vtkSMTrace::StartTrace();
  if (proxy)
  {
    trace->SetPropertiesToTraceOnCreate(
      vtkSMPropertyHelper(proxy, "PropertiesToTraceOnCreate").GetAsInt());
    trace->SetFullyTraceSupplementalProxies(
      vtkSMPropertyHelper(proxy, "FullyTraceSupplementalProxies").GetAsInt() == 1);
    if (vtkSMPropertyHelper(proxy, "ShowIncrementalTrace").GetAsInt() == 1)
    {
      pqCoreUtilities::connect(trace, vtkCommand::UpdateEvent, this, SLOT(updateTrace()));
    }
  }
}

//-----------------------------------------------------------------------------
void pqTraceReaction::stop()
{
  QString tracetxt(vtkSMTrace::StopTrace().c_str());
#ifdef PARAVIEW_ENABLE_PYTHON
  pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
  pythonManager->editTrace(tracetxt);
#endif
}

//-----------------------------------------------------------------------------
void pqTraceReaction::updateTrace()
{
#ifdef PARAVIEW_ENABLE_PYTHON
  pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
  pythonManager->editTrace(QString(), true);
#endif
}
