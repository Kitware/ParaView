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
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkCollection.h"
#include "vtkPVConfig.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

#include "vtkCTHData.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputProperty);
vtkCxxRevisionMacro(vtkPVInputProperty, "1.16");

//----------------------------------------------------------------------------
vtkPVInputProperty::vtkPVInputProperty()
{
  this->Name = NULL;
  this->Type = NULL;
}

//----------------------------------------------------------------------------
vtkPVInputProperty::~vtkPVInputProperty()
{  
  this->SetName(NULL);
  this->SetType(NULL);
}

//----------------------------------------------------------------------------
int vtkPVInputProperty::GetIsValidInput(vtkPVSource *input, vtkPVSource *pvs)
{
  // Used to be check if DisplayGui is NULL.
  if ( ! input->GetInitialized())
    {
    return 0;
    }

  vtkSMSourceProxy* proxy = pvs->GetProxy();
  if (!proxy)
    {
    vtkErrorMacro("The server manager prototype for " 
                  << pvs->GetSourceClassName()
                  << " does not exist.");
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
void vtkPVInputProperty::Copy(vtkPVInputProperty *in)
{
  this->SetName(in->GetName());
  this->SetType(in->GetType());
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


  



