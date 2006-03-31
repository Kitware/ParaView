/*=========================================================================

  Program:   ParaView
  Module:    vtkPVColorSelectionWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVColorSelectionWidget.h"

#include "vtkObjectFactory.h"
#include "vtkKWMenu.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVSource.h"
#include "vtkSMDataObjectDisplayProxy.h"

vtkStandardNewMacro(vtkPVColorSelectionWidget);
vtkCxxRevisionMacro(vtkPVColorSelectionWidget, "1.9");
//-----------------------------------------------------------------------------
vtkPVColorSelectionWidget::vtkPVColorSelectionWidget()
{
  this->ColorSelectionCommand = 0;
  this->Target = 0;
}

//-----------------------------------------------------------------------------
vtkPVColorSelectionWidget::~vtkPVColorSelectionWidget()
{
  this->SetColorSelectionCommand(0);
  this->SetPVSource(0);
  this->SetTarget(0);
}

//-----------------------------------------------------------------------------
void vtkPVColorSelectionWidget::Update(int remove_all /*=1*/)
{
  if (!this->PVSource)
    {
    vtkErrorMacro("PVSource must be set before calling Update.");
    return;
    }
  if (!this->Target)
    {
    vtkErrorMacro("Target must be set.");
    return;
    }
  if (!this->ColorSelectionCommand)
    {
    vtkErrorMacro("ColorSelectionCommand not set.");
    return;
    }
  if (remove_all)
    {
    this->GetMenu()->DeleteAllItems();
    }

  vtkPVDataSetAttributesInformation* attrInfo;
  vtkSMDisplayProxy* dproxy = this->PVSource->GetDisplayProxy();
  if (dproxy)
    {
    vtkPVDataInformation* geomInfo = dproxy->GetGeometryInformation();
    if (geomInfo)
      {

      attrInfo = geomInfo->GetPointDataInformation();
      this->AddArray(attrInfo, vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);

      attrInfo = geomInfo->GetCellDataInformation();
      this->AddArray(attrInfo, vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
      }
    }

}

//-----------------------------------------------------------------------------
void vtkPVColorSelectionWidget::AddArray(
  vtkPVDataSetAttributesInformation* attrInfo, int field_type)
{
  int numArrays = attrInfo->GetNumberOfArrays();
  int i;
  char label[350];
  char command[1024];
  int setFirstValue = 0;
  if (strcmp(this->GetValue(),"") == 0)
    {
    setFirstValue = 1;
    }

  for (i=0; i < numArrays; i++)
    {
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(i);
    sprintf(command, "%s {%s} %d",
      this->ColorSelectionCommand, arrayInfo->GetName(), field_type);
   
    if (!this->FormLabel(arrayInfo, field_type, label))
      {
      continue;
      }

    if (!this->GetMenu()->HasItem(label))
      {
      this->GetMenu()->AddRadioButton(label, this->Target,  command);
      if (setFirstValue)
        {
        this->SetValue(label);
        setFirstValue = 0;
        }
      }
    }
}

//-----------------------------------------------------------------------------
int vtkPVColorSelectionWidget::FormLabel(vtkPVArrayInformation* arrayInfo,
  int field, char *label)
{
  if (!arrayInfo)
    {
    vtkErrorMacro("Invalid arrayinfo.");
    return 0;
    }
  if (field != vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA &&
    field != vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
    {
    vtkErrorMacro("Field  must be POINT_FIELD_DATA or CELL_FIELD_DATA.");
    return 0;
    } 
  const char* pre_text = 
    (field == vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA)?
    "Point" : "Cell";
  int numComps = arrayInfo->GetNumberOfComponents();
  if (numComps > 1)
    {
    sprintf(label, "%s %s (%d)", pre_text, arrayInfo->GetName(), numComps);
    }
  else
    {
    sprintf(label, "%s %s", pre_text, arrayInfo->GetName());
    }
  return 1;
}

//-----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVColorSelectionWidget::GetArrayInformation(
  vtkPVDataInformation* dataInfo, const char* arrayname, int field)
{
  vtkPVDataSetAttributesInformation* attrInfo = 0;
  switch(field)
    {
    case vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA:
      attrInfo = dataInfo->GetPointDataInformation();
      break;
    case vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA:
      attrInfo = dataInfo->GetCellDataInformation();
      break;
    default:
      vtkErrorMacro("Field type " << field << " not supported.");
      return 0;
    }

  if (attrInfo)
    {
    return attrInfo->GetArrayInformation(arrayname);
    }
  vtkErrorMacro("Attribute information does not exist. Returning null.");
  return 0;
}

//-----------------------------------------------------------------------------
void vtkPVColorSelectionWidget::SetValue(const char* arrayname, int field)
{
  char label[350];
  vtkPVDataInformation* dataInfo = this->PVSource->GetDataInformation();
  vtkPVArrayInformation* aInfo = 
    this->GetArrayInformation(dataInfo, arrayname, field);

  // If the array is not found in the data information, look at
  // the geometry information.
  if (!aInfo)
    {
    vtkSMDisplayProxy* dproxy = this->PVSource->GetDisplayProxy();
    if (dproxy)
      {
      vtkPVDataInformation* geomInfo = dproxy->GetGeometryInformation();
      aInfo = 
        this->GetArrayInformation(geomInfo, arrayname, field);
      }
    }
  if (!this->FormLabel(aInfo, field, label))
    {
    return;
    }
  this->SetValue(label);
}

//-----------------------------------------------------------------------------
void vtkPVColorSelectionWidget::SetValue(const char* val)
{
  this->Superclass::SetValue(val);
}

//-----------------------------------------------------------------------------
void vtkPVColorSelectionWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Target: " << this->Target << endl;
  os << indent << "ColorSelectionCommand: " << this->ColorSelectionCommand << endl;
}
