/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataSetFileEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataSetFileEntry.h"

#include "vtkPDataSetReader.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDataSetFileEntry);
vtkCxxRevisionMacro(vtkPVDataSetFileEntry, "1.12");

//----------------------------------------------------------------------------
vtkPVDataSetFileEntry::vtkPVDataSetFileEntry()
{
  this->TypeReader = vtkPDataSetReader::New();
  this->Type = -1;
}

//----------------------------------------------------------------------------
vtkPVDataSetFileEntry::~vtkPVDataSetFileEntry()
{
  this->TypeReader->Delete();
  this->TypeReader = NULL;
}



//----------------------------------------------------------------------------
void vtkPVDataSetFileEntry::AcceptInternal(vtkClientServerID sourceID)
{
  int newType;

  this->TypeReader->SetFileName(this->GetValue());
  newType = this->TypeReader->ReadOutputType();

  if (newType == -1)
    {
    this->GetPVApplication()->GetMainWindow()->WarningMessage(
      "Could not read file.");
    this->ResetInternal();
    return;
    }
  if (this->Type == -1)
    {
    this->Type = newType;
    }
  if (this->Type != newType)
    {
    this->GetPVApplication()->GetMainWindow()->WarningMessage(
      "Output type does not match previous file.");
    this->ResetInternal();
    return;
    }

  this->vtkPVFileEntry::AcceptInternal(sourceID);
}


//----------------------------------------------------------------------------
vtkPVDataSetFileEntry* vtkPVDataSetFileEntry::ClonePrototype(
  vtkPVSource* pvSource, vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVDataSetFileEntry::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
int vtkPVDataSetFileEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                             vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVDataSetFileEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
