/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInputProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkCollection.h"
#include "vtkPVInputRequirement.h"
#include "vtkPVConfig.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

#include "vtkCTHData.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputProperty);
vtkCxxRevisionMacro(vtkPVInputProperty, "1.12");

//----------------------------------------------------------------------------
vtkPVInputProperty::vtkPVInputProperty()
{
  this->Name = NULL;
  this->Type = NULL;
  this->Requirements = vtkCollection::New();
}

//----------------------------------------------------------------------------
vtkPVInputProperty::~vtkPVInputProperty()
{  
  this->SetName(NULL);
  this->SetType(NULL);
  this->Requirements->Delete();
  this->Requirements = NULL;
}

//----------------------------------------------------------------------------
void vtkPVInputProperty::AddRequirement(vtkPVInputRequirement *ir)
{
  this->Requirements->AddItem(ir);
}

//----------------------------------------------------------------------------
int vtkPVInputProperty::GetIsValidInput(vtkPVSource *input, vtkPVSource *pvs)
{
  if (input->GetPVOutput() == NULL)
    {
    return 0;
    }

  vtkSMSourceProxy* proxy = pvs->GetProxy();
  if (!proxy)
    {
    //cout << "< " << pvs->GetSourceClassName() << endl;
    return 0;
    }
  vtkSMProxyProperty* property = vtkSMProxyProperty::SafeDownCast(
    proxy->GetProperty(this->GetName()));
  if (!property)
    {
    //cout << ">" << this->GetName() << endl;
    return 0;
    }
  property->RemoveAllUncheckedProxies();
  property->AddUncheckedProxy(input->GetProxy());
  
  return property->IsInDomains();
}

//----------------------------------------------------------------------------
int vtkPVInputProperty::GetIsValidField(
  int field, vtkPVDataSetAttributesInformation* fieldInfo)
{
  vtkPVInputRequirement *ir;

  // First check the field has the correct arrays.
  this->Requirements->InitTraversal();
  while ( (ir = 
           (vtkPVInputRequirement*)(this->Requirements->GetNextItemAsObject())))
    {
    if ( ! ir->GetIsValidField(field, fieldInfo) )
      {
      return 0;
      }
    }

  return 1;
}



//----------------------------------------------------------------------------
void vtkPVInputProperty::Copy(vtkPVInputProperty *in)
{
  this->SetName(in->GetName());
  this->SetType(in->GetType());
  this->Requirements->RemoveAllItems();
  
  vtkPVInputRequirement *ir;
  in->Requirements->InitTraversal();
  while ( (ir = 
           (vtkPVInputRequirement*)(in->Requirements->GetNextItemAsObject())))
    {
    this->AddRequirement(ir);
    }
}

//----------------------------------------------------------------------------
void vtkPVInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if (this->Name)
    {
    os << indent << "Name: " << this->Name << endl;
    }
  if (this->Type)
    {
    os << indent << "Type: " << this->Type << endl;
    }
}


  



