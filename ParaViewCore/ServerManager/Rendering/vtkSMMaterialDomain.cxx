/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMaterialDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMMaterialDomain.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVMaterialLibrary.h"
#include "vtkSMMaterialLibraryProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkStdString.h"
#include "vtkWeakPointer.h"

#include "vtkPVConfig.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayMaterialLibrary.h"
#endif

class vtkSMMaterialObserver : public vtkCommand
{
public:
  static vtkSMMaterialObserver* New()
  {
    vtkSMMaterialObserver* ret = new vtkSMMaterialObserver;
    return ret;
  }
  vtkTypeMacro(vtkSMMaterialObserver, vtkCommand);
  vtkSMMaterialObserver()
  {
    this->Owner = nullptr;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
    this->Watchee = nullptr;
#endif
  }
  ~vtkSMMaterialObserver() {}

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventid),
    void* vtkNotUsed(calldata)) override
  {
    this->Owner->CallMeSometime();
  }

  void StartWatching()
  {
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
    vtkNew<vtkSMParaViewPipelineController> controller;
    vtkSMMaterialLibraryProxy* mlp = vtkSMMaterialLibraryProxy::SafeDownCast(
      controller->FindMaterialLibrary(this->Owner->GetSession()));
    if (mlp)
    {
      // todo: is GetClientSideObject guaranteed to succeed?
      this->Watchee = vtkOSPRayMaterialLibrary::SafeDownCast(
        vtkPVMaterialLibrary::SafeDownCast(mlp->GetClientSideObject())->GetMaterialLibrary());
      if (this->Watchee)
      {
        this->Watchee->AddObserver(vtkCommand::UpdateDataEvent, this);
      }
    }
#endif
  }
  void StopWatching()
  {
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
    if (this->Watchee)
    {
      this->Watchee->RemoveObserver(this);
    }
#endif
  }
  vtkWeakPointer<vtkSMMaterialDomain> Owner;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkWeakPointer<vtkOSPRayMaterialLibrary> Watchee;
#endif
};

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMMaterialDomain);

//---------------------------------------------------------------------------
vtkSMMaterialDomain::vtkSMMaterialDomain()
{
  this->Observer = vtkSMMaterialObserver::New();
  this->Observer->Owner = this;
}

//---------------------------------------------------------------------------
vtkSMMaterialDomain::~vtkSMMaterialDomain()
{
  this->Observer->StopWatching();
  this->Observer->Delete();
}

//---------------------------------------------------------------------------
void vtkSMMaterialDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
int vtkSMMaterialDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  int retVal = this->Superclass::ReadXMLAttributes(prop, element);
  if (!retVal)
  {
    return 0;
  }
  // throw away whatever XML said our strings are and call update instead
  this->Update(nullptr);

  // register to  update self whenever available materials change
  this->Observer->StartWatching();
#else
  (void)prop;
  (void)element;
#endif
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMMaterialDomain::CallMeSometime()
{
  // materials changed, update self with new contents
  this->Update(nullptr);
  this->DomainModified();
}

//---------------------------------------------------------------------------
void vtkSMMaterialDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  // find the material library we reflect contents of
  vtkNew<vtkSMParaViewPipelineController> controller;
  vtkSMMaterialLibraryProxy* mlp =
    vtkSMMaterialLibraryProxy::SafeDownCast(controller->FindMaterialLibrary(this->GetSession()));
  if (!mlp)
  {
    return;
  }
  vtkOSPRayMaterialLibrary* ml = vtkOSPRayMaterialLibrary::SafeDownCast(
    vtkPVMaterialLibrary::SafeDownCast(mlp->GetClientSideObject())->GetMaterialLibrary());
  if (!ml)
  {
    return;
  }

  // populate my list
  std::vector<vtkStdString> sa;
  sa.push_back(vtkStdString("None")); // 1: standard vtk coloration
  // 2: whole actor material choices
  std::set<std::string> materialNames = ml->GetMaterialNames();
  std::set<std::string>::iterator it = materialNames.begin();
  while (it != materialNames.end())
  {
    sa.push_back(*it);
    it++;
  }
  // 3: cells/blocks can choose for themselves from the above
  if (materialNames.size() > 1)
  {
    sa.push_back(vtkStdString("Value Indexed"));
  }

  this->SetStrings(sa);
#endif
}
