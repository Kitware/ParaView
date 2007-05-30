/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiViewFactory.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiViewFactory.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMMultiViewFactory);
vtkCxxRevisionMacro(vtkSMMultiViewFactory, "1.1");
//----------------------------------------------------------------------------
vtkSMMultiViewFactory::vtkSMMultiViewFactory()
{
  this->RenderViewName = 0;
}

//----------------------------------------------------------------------------
vtkSMMultiViewFactory::~vtkSMMultiViewFactory()
{
  this->SetRenderViewName(0);
}

//----------------------------------------------------------------------------
void vtkSMMultiViewFactory::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  if (!this->RenderViewName)
    {
    vtkErrorMacro("A render view name has to be set before "
      "vtkSMMultiViewFactory can create vtk objects.");
    return;
    }

  this->Superclass::CreateVTKObjects();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMMultiViewFactory::NewRenderView()
{
  this->CreateVTKObjects();

  if (!this->ObjectsCreated)
    {
    return 0;
    }

  vtkSMProxy* view = this->GetProxyManager()->NewProxy(
    "newviews", this->RenderViewName);
  if (view)
    {
    view->SetConnectionID(this->ConnectionID);
    }
  return view;
}

//----------------------------------------------------------------------------
void vtkSMMultiViewFactory::AddRenderView(vtkSMProxy* vtkNotUsed(view))
{
  // Set shared objects.
}

//----------------------------------------------------------------------------
void vtkSMMultiViewFactory::RemoveRenderView(vtkSMProxy* vtkNotUsed(view))
{
  // Unset shared objects.
}

//----------------------------------------------------------------------------
void vtkSMMultiViewFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderViewName: " 
    << (this->RenderViewName? this->RenderViewName : "(none)") << endl;

}


