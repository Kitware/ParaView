/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTextSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTextSource.h"

#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkStringArray.h"

vtkStandardNewMacro(vtkPVTextSource);
//----------------------------------------------------------------------------
vtkPVTextSource::vtkPVTextSource()
{
  this->Text = 0;
  this->SetNumberOfInputPorts(0);  
}

//----------------------------------------------------------------------------
vtkPVTextSource::~vtkPVTextSource()
{
  this->SetText(0);
}

//----------------------------------------------------------------------------
int vtkPVTextSource::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* vtkNotUsed(info))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVTextSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), 
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkTable* output = this->GetOutput();

  vtkStringArray * data = vtkStringArray::New();
  data->SetName("Text");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(this->Text? this->Text : "");
  output->AddColumn(data);
  data->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVTextSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Text: " << (this->Text? this->Text : "(none)") << endl;
}


