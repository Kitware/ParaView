/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiViewManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiViewManager.h"

#include "vtkObjectFactory.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>

class vtkMultiViewManager::vtkRendererMap : 
  public vtkstd::map<int, vtkSmartPointer<vtkRendererCollection> >
{
};


vtkStandardNewMacro(vtkMultiViewManager);
vtkCxxRevisionMacro(vtkMultiViewManager, "1.1");
//----------------------------------------------------------------------------
vtkMultiViewManager::vtkMultiViewManager()
{
  this->GUISize[0] = this->GUISize[1] = 400;
  this->WindowSize[0] = this->WindowSize[1] = 400;
  this->WindowPosition[0] = this->WindowPosition[1] = 0;
  this->RenderWindow = 0;

  this->ActiveViewID = 0;
  this->RendererMap = new vtkRendererMap();

  vtkMemberFunctionCommand<vtkMultiViewManager>* obs = 
    vtkMemberFunctionCommand<vtkMultiViewManager>::New();
  obs->SetCallback(*this, &vtkMultiViewManager::StartRenderCallback);
  this->Observer = obs;
}

//----------------------------------------------------------------------------
vtkMultiViewManager::~vtkMultiViewManager()
{
  this->SetRenderWindow(0);
  this->Observer->Delete();

  delete this->RendererMap;
  this->RendererMap = 0;
}

//----------------------------------------------------------------------------
void vtkMultiViewManager::SetRenderWindow(vtkRenderWindow* win)
{
  if (this->RenderWindow)
    {
    this->RenderWindow->RemoveObserver(this->Observer);
    }

  vtkSetObjectBodyMacro(RenderWindow, vtkRenderWindow, win);
  
  if (this->RenderWindow)
    {
    // We want our event handler to be called before the event handler in the
    // parallel render manager.
    this->RenderWindow->AddObserver(vtkCommand::StartEvent,
      this->Observer, 100);
    }
}

//----------------------------------------------------------------------------
vtkRendererCollection* vtkMultiViewManager::GetActiveRenderers()
{
  return this->GetRenderers(this->ActiveViewID);
}

//----------------------------------------------------------------------------
vtkRendererCollection* vtkMultiViewManager::GetRenderers(int id)
{
  vtkRendererMap::iterator iter = this->RendererMap->find(id);
  if (iter != this->RendererMap->end())
    {
    return iter->second.GetPointer();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkMultiViewManager::StartRenderCallback()
{
  double viewport[4];
  viewport[0] = this->WindowPosition[0]/(double)this->GUISize[0];
  viewport[1] = this->WindowPosition[1]/(double)this->GUISize[1];
  viewport[2] = (this->WindowPosition[0]+this->WindowSize[0])/
    (double)this->GUISize[0];
  viewport[3] = (this->WindowPosition[1]+this->WindowSize[1])/
    (double)this->GUISize[1];

 
  vtkRendererCollection* renderers = this->GetActiveRenderers();

  if (!renderers)
    {
    vtkErrorMacro("No active renderers selected!");
    return;
    }

  renderers->InitTraversal();
  while (vtkRenderer* ren = renderers->GetNextItem())
    {
    ren->DrawOn();
    ren->SetViewport(viewport);
    }

  // TODO: we may want to disable all other renders present in the render
  // window.
}

//----------------------------------------------------------------------------
void vtkMultiViewManager::AddRenderer(int id, vtkRenderer* ren)
{
  vtkRendererMap::iterator iter = this->RendererMap->find(id);
  if (iter== this->RendererMap->end())
    {
    (*this->RendererMap)[id] = vtkSmartPointer<vtkRendererCollection>::New();
    iter = this->RendererMap->find(id);
    }
  iter->second.GetPointer()->AddItem(ren);
}

//----------------------------------------------------------------------------
void vtkMultiViewManager::RemoveRenderer(int id, vtkRenderer* ren)
{
  vtkRendererMap::iterator iter = this->RendererMap->find(id);
  if (iter!= this->RendererMap->end())
    {
    iter->second.GetPointer()->RemoveItem(ren);
    }
}

//----------------------------------------------------------------------------
void vtkMultiViewManager::RemoveAllRenderers(int id)
{
  vtkRendererMap::iterator iter = this->RendererMap->find(id);
  if (iter!= this->RendererMap->end())
    {
    this->RendererMap->erase(iter);
    }
}

//----------------------------------------------------------------------------
void vtkMultiViewManager::RemoveAllRenderers()
{
  this->RendererMap->clear();
}

//----------------------------------------------------------------------------
void vtkMultiViewManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GUISize: " 
    << this->GUISize[0] << ", " << this->GUISize[1] << endl;
  os << indent << "WindowPosition: " 
    << this->WindowPosition[0] << ", " << this->WindowPosition[1] << endl;
  os << indent << "WindowSize: " 
    << this->WindowSize[0] << ", " << this->WindowSize[1] << endl;
}


