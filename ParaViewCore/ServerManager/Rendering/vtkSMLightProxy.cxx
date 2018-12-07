/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLightProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLightProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVLight.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <assert.h>

class vtkSMLightObserver : public vtkCommand
{
public:
  static vtkSMLightObserver* New()
  {
    vtkSMLightObserver* ret = new vtkSMLightObserver;
    return ret;
  }
  vtkTypeMacro(vtkSMLightObserver, vtkCommand);

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventid),
    void* vtkNotUsed(calldata)) override
  {
    this->Owner->PropertyChanged();
  }
  vtkSMLightProxy* Owner;
};

vtkStandardNewMacro(vtkSMLightProxy);
//----------------------------------------------------------------------------
vtkSMLightProxy::vtkSMLightProxy()
  : Observer(nullptr)
{
}

//----------------------------------------------------------------------------
vtkSMLightProxy::~vtkSMLightProxy()
{
  if (this->Observer)
  {
    this->Observer->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkSMLightProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMLightProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();
  this->Observer = vtkSMLightObserver::New();
  this->Observer->Owner = this;
  vtkSMProperty* typeProperty = this->GetProperty("LightType");
  typeProperty->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
}

//----------------------------------------------------------------------------
void vtkSMLightProxy::PropertyChanged()
{
  // if the light type changes, update the position/focal to correspond.
  int type = 0;
  vtkSMPropertyHelper(this, "LightType").Get(&type);
  // default camera light params.
  double position[3] = { 0, 0, 1 };
  double focal[3] = { 0, 0, 0 };
  // nothing to do for headlight or ambient light
  if (type == VTK_LIGHT_TYPE_CAMERA_LIGHT)
  {
    // camera light is relative the camera already, reset to defaults.
    vtkSMPropertyHelper(this, "LightPosition").Modified().Set(position, 3);
    vtkSMPropertyHelper(this, "FocalPoint").Modified().Set(focal, 3);
  }
  else if (type == VTK_LIGHT_TYPE_SCENE_LIGHT)
  {
    // pick up whatever position is in the vtk object.
    // for a headlight, this will match the scene coords. It won't for a camera light.
    // We can't match a camera light's scene coords without its transform, which is already gone.
    this->UpdatePropertyInformation();
    vtkSMPropertyHelper(this, "PositionInfo").Get(position, 3);
    vtkSMPropertyHelper(this, "FocalPointInfo").Get(focal, 3);
    vtkSMPropertyHelper(this, "LightPosition").Modified().Set(position, 3);
    vtkSMPropertyHelper(this, "FocalPoint").Modified().Set(focal, 3);
  }
}
