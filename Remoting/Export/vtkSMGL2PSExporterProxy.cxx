/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGL2PSExporterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMGL2PSExporterProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVGL2PSExporter.h"
#include "vtkPVXMLElement.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"

vtkStandardNewMacro(vtkSMGL2PSExporterProxy)

  //----------------------------------------------------------------------------
  vtkSMGL2PSExporterProxy::vtkSMGL2PSExporterProxy()
  : ViewType(None)
{
}

//----------------------------------------------------------------------------
vtkSMGL2PSExporterProxy::~vtkSMGL2PSExporterProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMGL2PSExporterProxy::CanExport(vtkSMProxy* proxy)
{
  return proxy && ((this->ViewType == RenderView && proxy->IsA("vtkSMRenderViewProxy")) ||
                    (this->ViewType == ContextView && proxy->IsA("vtkSMContextViewProxy")));
}

//----------------------------------------------------------------------------
void vtkSMGL2PSExporterProxy::Write()
{
  this->CreateVTKObjects();

  vtkPVGL2PSExporter* exporter = vtkPVGL2PSExporter::SafeDownCast(this->GetClientSideObject());

  vtkSMRenderViewProxy* rv =
    this->ViewType == RenderView ? vtkSMRenderViewProxy::SafeDownCast(this->View) : NULL;
  vtkSMContextViewProxy* cv =
    this->ViewType == ContextView ? vtkSMContextViewProxy::SafeDownCast(this->View) : NULL;

  vtkRenderWindow* renWin = NULL;
  if (rv)
  {
    renWin = rv->GetRenderWindow();
  }
  else if (cv)
  {
    renWin = cv->GetRenderWindow();
  }

  if (exporter && renWin)
  {
    exporter->SetRenderWindow(renWin);
    exporter->Write();
    exporter->SetRenderWindow(NULL);
  }
}

//----------------------------------------------------------------------------
int vtkSMGL2PSExporterProxy::ReadXMLAttributes(
  vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  const char* viewType(element->GetAttribute("viewtype"));
  this->ViewType = None;
  if (viewType)
  {
    if (strcmp(viewType, "none") == 0)
    {
      // This proxy definition just defines a base interface.
      return 0;
    }
    if (strcmp(viewType, "renderview") == 0)
    {
      this->ViewType = RenderView;
    }
    else if (strcmp(viewType, "contextview") == 0)
    {
      this->ViewType = ContextView;
    }
    else
    {
      vtkErrorMacro(<< "Invalid viewtype specified: " << viewType);
      return 0;
    }
  }
  else
  {
    vtkErrorMacro(<< "No viewtype specified.");
    return 0;
  }

  return this->Superclass::ReadXMLAttributes(pm, element);
}

//----------------------------------------------------------------------------
void vtkSMGL2PSExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ViewType: ";
  switch (ViewType)
  {
    case RenderView:
      os << "RenderView";
      break;
    case ContextView:
      os << "RenderView";
      break;
    default:
      os << "Unknown";
      break;
  }
  os << endl;
}
