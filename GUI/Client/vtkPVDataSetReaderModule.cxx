/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataSetReaderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataSetReaderModule.h"

#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPDataSetReader.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"

#include <ctype.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDataSetReaderModule);
vtkCxxRevisionMacro(vtkPVDataSetReaderModule, "1.22");

//----------------------------------------------------------------------------
void vtkPVDataSetReaderModule::CreateProperties()
{
}

//----------------------------------------------------------------------------
void vtkPVDataSetReaderModule::InitializePrototype()
{
  this->Superclass::InitializePrototype();
}

//----------------------------------------------------------------------------
int vtkPVDataSetReaderModule::Initialize(const char*, vtkPVReaderModule*&)
{

  return VTK_OK;
}


//----------------------------------------------------------------------------
void vtkPVDataSetReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
