/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExporterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExporterProxy.h"

#include "vtkExporter.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMultiProcessRenderView.h"

vtkStandardNewMacro(vtkSMExporterProxy);
vtkCxxRevisionMacro(vtkSMExporterProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMExporterProxy::vtkSMExporterProxy()
{
  this->View = 0;
  this->FileExtension = 0;
  this->SetFileExtension("txt");
  this->SetServers(vtkProcessModule::CLIENT);
}

//----------------------------------------------------------------------------
vtkSMExporterProxy::~vtkSMExporterProxy()
{
  this->SetView(0);
  this->SetFileExtension(0);
}

//----------------------------------------------------------------------------
bool vtkSMExporterProxy::CanExport(vtkSMProxy* view)
{
  return (view && view->IsA("vtkSMRenderViewProxy"));
}

//----------------------------------------------------------------------------
void vtkSMExporterProxy::SetView(vtkSMRenderViewProxy* view)
{
  vtkSetObjectBodyMacro(View, vtkSMRenderViewProxy, view);
  if (this->ObjectsCreated)
    {
    vtkRenderWindow* renWin = this->View? this->View->GetRenderWindow() : 0;
    vtkExporter* exporter = vtkExporter::SafeDownCast(this->GetClientSideObject());
    if (exporter)
      {
      exporter->SetRenderWindow(renWin);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMExporterProxy::Write()
{
  vtkExporter* exporter = vtkExporter::SafeDownCast(this->GetClientSideObject());
  if (exporter && this->View)
    {
    vtkSMMultiProcessRenderView* mrv = 
      vtkSMMultiProcessRenderView::SafeDownCast(this->View);
    double old_threshold = 0.0;
    if (mrv)
      {
      old_threshold = mrv->GetRemoteRenderThreshold();
      mrv->SetRemoteRenderThreshold(VTK_DOUBLE_MAX);
      mrv->StillRender();
      }
    exporter->Write();
    if (mrv)
      {
      mrv->SetRemoteRenderThreshold(old_threshold);
      }
    }
}

//----------------------------------------------------------------------------
int vtkSMExporterProxy::ReadXMLAttributes(
  vtkSMProxyManager* pxm, vtkPVXMLElement* element)
{
  const char* exts = element->GetAttribute("file_extension");
  if (exts)
    {
    this->SetFileExtension(exts);
    }

  return this->Superclass::ReadXMLAttributes(pxm, element);
}

//----------------------------------------------------------------------------
void vtkSMExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "View: " << this->View << endl;
  os << indent << "FileExtension: " 
    << (this->FileExtension? this->FileExtension : "(none)") << endl;
}


