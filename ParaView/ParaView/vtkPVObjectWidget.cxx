/*=========================================================================

  Program:   ParaView
  Module:    vtkPVObjectWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVObjectWidget.h"

#include "vtkArrayMap.txx"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkString.h"
#include "vtkPVProcessModule.h"
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkPVObjectWidget, "1.14");

//----------------------------------------------------------------------------
vtkPVObjectWidget::vtkPVObjectWidget()
{
  this->ObjectID.ID = 0;
  this->VariableName = NULL;
}

//----------------------------------------------------------------------------
vtkPVObjectWidget::~vtkPVObjectWidget()
{
  this->SetVariableName(NULL);
}


//----------------------------------------------------------------------------
void vtkPVObjectWidget::SaveInBatchScriptForPart(ofstream *file,
                                                 vtkClientServerID sourceID)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule(); 
 
  if (sourceID.ID == 0 || this->VariableName == NULL)
    {
    vtkErrorMacro(<< this->GetClassName()
                  << " must not have SaveInBatchScript method.");
    return;
    } 

  *file << "\t" << "pvTemp" << sourceID << " Set" << this->VariableName;
  ostrstream str;
  str << "Get" << this->VariableName << ends;
  pm->GetStream() << vtkClientServerStream::Invoke << this->ObjectID
                  << str.str() 
                  << vtkClientServerStream::End;
  pm->SendStreamToClient();
  ostrstream result;
  pm->GetLastClientResult().PrintArgumentValue(result, 0,0);
  result << ends;
  *file << " " << result.str() << "\n";
  delete [] result.str();
  delete [] str.str();
}

//----------------------------------------------------------------------------
vtkPVObjectWidget* vtkPVObjectWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVObjectWidget::SafeDownCast(clone);
}

//-----------------------------------------------------------------------------
void vtkPVObjectWidget::CopyProperties(vtkPVWidget* clone, 
                                       vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVObjectWidget* pvow = vtkPVObjectWidget::SafeDownCast(clone);
  if (pvow)
    {
    pvow->SetVariableName(this->VariableName);
    pvow->SetObjectID(pvSource->GetVTKSourceID());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVObjectWidget.");
    }
}

//----------------------------------------------------------------------------
int vtkPVObjectWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                          vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  const char* variable = element->GetAttribute("variable");
  if(variable)
    {
    this->SetVariableName(variable);
    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkPVObjectWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "ObjectID: " << (this->ObjectID.ID)
     << endl;
  os << "VariableName: " << (this->VariableName?this->VariableName:"none") 
     << endl;
}
