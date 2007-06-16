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
#include "vtkSmartPointer.h"
#include "vtkSMViewProxy.h"

#include <vtksys/ios/sstream>
#include <vtkstd/vector>

class vtkSMMultiViewFactory::vtkVector: 
  public vtkstd::vector<vtkSmartPointer<vtkSMViewProxy> > {};

vtkStandardNewMacro(vtkSMMultiViewFactory);
vtkCxxRevisionMacro(vtkSMMultiViewFactory, "1.2.2.1");
//----------------------------------------------------------------------------
vtkSMMultiViewFactory::vtkSMMultiViewFactory()
{
  this->RenderViewName = 0;
  this->RenderViews = new vtkSMMultiViewFactory::vtkVector();
}

//----------------------------------------------------------------------------
vtkSMMultiViewFactory::~vtkSMMultiViewFactory()
{
  this->SetRenderViewName(0);
  delete this->RenderViews;
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
vtkSMViewProxy* vtkSMMultiViewFactory::NewRenderView()
{
  this->CreateVTKObjects();

  if (!this->ObjectsCreated)
    {
    return 0;
    }

  vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(
    this->GetProxyManager()->NewProxy("newviews", 
      this->RenderViewName));
  if (view)
    {
    view->SetConnectionID(this->ConnectionID);
    }
  return view;
}

//----------------------------------------------------------------------------
void vtkSMMultiViewFactory::AddRenderView(vtkSMViewProxy* view)
{
  // Set shared objects.
  this->RenderViews->push_back(view);

  if (this->RenderViews->size() > 1 && view)
    {
    vtkSMViewProxy* root = this->RenderViews->front().GetPointer();
    view->InitializeForMultiView(root);
    }
}

//----------------------------------------------------------------------------
void vtkSMMultiViewFactory::RemoveRenderView(vtkSMViewProxy* view)
{
  // Unset shared objects.
  
  vtkSMMultiViewFactory::vtkVector::iterator iter;
  for (iter = this->RenderViews->begin(); 
    iter != this->RenderViews->end(); ++iter)
    {
    if (iter->GetPointer() == view)
      {
      this->RenderViews->erase(iter);
      break;
      }
    }
}

//----------------------------------------------------------------------------
unsigned int vtkSMMultiViewFactory::GetNumberOfRenderViews()
{
  return this->RenderViews->size();
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMMultiViewFactory::GetRenderView(unsigned int cc)
{
  if (cc < this->RenderViews->size())
    {
    return (*this->RenderViews)[cc];
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkSMMultiViewFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderViewName: " 
    << (this->RenderViewName? this->RenderViewName : "(none)") << endl;

}


