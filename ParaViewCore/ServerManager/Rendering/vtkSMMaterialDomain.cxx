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
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkStdString.h"

class vtkSMMaterialObserver : public vtkCommand
{
public:
  static vtkSMMaterialObserver* New() { return new vtkSMMaterialObserver; }
  vtkTypeMacro(vtkSMMaterialObserver, vtkCommand);

  void Execute(vtkObject* caller, unsigned long eventid, void* calldata) VTK_OVERRIDE
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
  vtkNew<vtkSMMaterialObserver> observer;
  observer->Owner = this;
  vtkOSPRayMaterialLibrary::GetInstance()->AddObserver(vtkCommand::UpdateDataEvent, observer.Get());
}

//---------------------------------------------------------------------------
vtkSMMaterialDomain::~vtkSMMaterialDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMMaterialDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
int vtkSMMaterialDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  int retVal = this->Superclass::ReadXMLAttributes(prop, element);
  if (!retVal)
  {
    return 0;
  }
  // throw away whatever XML said our strings are and call update instead
  this->Update(nullptr);
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMMaterialDomain::CallMeSometime()
{
  this->Update(nullptr);
  this->DomainModified();
}

//---------------------------------------------------------------------------
void vtkSMMaterialDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{
  std::vector<vtkStdString> sa;
  sa.push_back(vtkStdString("None"));

  std::set<std::string> materialNames = vtkOSPRayMaterialLibrary::GetInstance()->GetMaterialNames();
  std::set<std::string>::iterator it = materialNames.begin();
  while (it != materialNames.end())
  {
    sa.push_back(*it);
    it++;
  }
  if (materialNames.size() > 1)
  {
    sa.push_back(vtkStdString("MasterMaterial"));
  }
  this->SetStrings(sa);
}
