/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRawReaderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRawReaderModule.h"

#include "vtkObjectFactory.h"
#include "vtkPVFileEntry.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRawReaderModule);
vtkCxxRevisionMacro(vtkPVRawReaderModule, "1.5");

int vtkPVRawReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVRawReaderModule::vtkPVRawReaderModule()
{
}

//----------------------------------------------------------------------------
vtkPVRawReaderModule::~vtkPVRawReaderModule()
{
}

//----------------------------------------------------------------------------
void vtkPVRawReaderModule::CreateProperties()
{
  this->Superclass::CreateProperties();

  this->FileEntry->SetLabel("File Prefix");
  this->FileEntry->SetSMPropertyName("FilePrefix");
}

//----------------------------------------------------------------------------
void vtkPVRawReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
