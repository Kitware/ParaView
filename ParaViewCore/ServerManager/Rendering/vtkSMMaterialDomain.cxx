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
#include "vtkSMMaterialLibraryProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkStdString.h"

#include "vtkPVConfig.h"

#ifdef PARAVIEW_USE_OSPRAY
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

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventid),
    void* vtkNotUsed(calldata)) VTK_OVERRIDE
  {
    this->Owner->CallMeSometime();
  }
  vtkSMMaterialDomain* Owner;
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
#ifdef PARAVIEW_USE_OSPRAY
  int retVal = this->Superclass::ReadXMLAttributes(prop, element);
  if (!retVal)
  {
    return 0;
  }
  // throw away whatever XML said our strings are and call update instead
  this->Update(nullptr);

  // register updates whenever the materials change
  vtkNew<vtkSMParaViewPipelineController> controller;
  vtkSMMaterialLibraryProxy* mlp =
    vtkSMMaterialLibraryProxy::SafeDownCast(controller->FindMaterialLibrary(this->GetSession()));
  if (!mlp)
  {
    return 1;
  }
  // todo: is GetClientSideObject guaranteed to succeed?
  vtkOSPRayMaterialLibrary* ml = vtkOSPRayMaterialLibrary::SafeDownCast(mlp->GetClientSideObject());
  if (!ml)
  {
    return 1;
  }
  ml->AddObserver(vtkCommand::UpdateDataEvent, this->Observer);
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
#ifdef PARAVIEW_USE_OSPRAY
  // find the material library we reflect contents of
  vtkNew<vtkSMParaViewPipelineController> controller;
  vtkSMMaterialLibraryProxy* mlp =
    vtkSMMaterialLibraryProxy::SafeDownCast(controller->FindMaterialLibrary(this->GetSession()));
  if (!mlp)
  {
    return;
  }
  vtkOSPRayMaterialLibrary* ml = vtkOSPRayMaterialLibrary::SafeDownCast(mlp->GetClientSideObject());
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
    sa.push_back(vtkStdString("MasterMaterial"));
  }

  this->SetStrings(sa);
#endif
}
