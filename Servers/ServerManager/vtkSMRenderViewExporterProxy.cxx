/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewExporterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRenderViewExporterProxy.h"

#include "vtkExporter.h"
#include "vtkObjectFactory.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMMultiProcessRenderView.h"

vtkStandardNewMacro(vtkSMRenderViewExporterProxy);
//----------------------------------------------------------------------------
vtkSMRenderViewExporterProxy::vtkSMRenderViewExporterProxy()
{
}

//----------------------------------------------------------------------------
vtkSMRenderViewExporterProxy::~vtkSMRenderViewExporterProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewExporterProxy::CanExport(vtkSMProxy* view)
{
  return (view && view->IsA("vtkSMRenderViewProxy"));
}

//----------------------------------------------------------------------------
void vtkSMRenderViewExporterProxy::Write()
{
  this->CreateVTKObjects();

  vtkExporter* exporter = vtkExporter::SafeDownCast(this->GetClientSideObject());
  vtkSMRenderViewProxy* rv = vtkSMRenderViewProxy::SafeDownCast(this->View);
  if (exporter && rv)
    {
    vtkSMMultiProcessRenderView* mrv = 
      vtkSMMultiProcessRenderView::SafeDownCast(rv);
    double old_threshold = 0.0;
    if (mrv)
      {
      old_threshold = mrv->GetRemoteRenderThreshold();
      mrv->SetRemoteRenderThreshold(VTK_DOUBLE_MAX);
      mrv->StillRender();
      }

    vtkRenderWindow* renWin = rv->GetRenderWindow();
    exporter->SetRenderWindow(renWin);
    exporter->Write();
    exporter->SetRenderWindow(0);
    if (mrv)
      {
      mrv->SetRemoteRenderThreshold(old_threshold);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRenderViewExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


