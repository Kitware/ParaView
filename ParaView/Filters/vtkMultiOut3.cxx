/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiOut3.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiOut3.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkCharArray.h"
#include "vtkFieldData.h"
#include "vtkImageMandelbrotSource.h"

#include <math.h>

vtkCxxRevisionMacro(vtkMultiOut3, "1.2");
vtkStandardNewMacro(vtkMultiOut3);

//------------------------------------------------------------------------------
vtkMultiOut3::vtkMultiOut3()
{
  vtkImageData *image;

  this->SetNumberOfOutputs(2);

  image = vtkImageData::New();
  this->SetNthOutput(0, image);
  image->Delete();

  image = vtkImageData::New();
  this->SetNthOutput(1, image);
  image->Delete();

}

//------------------------------------------------------------------------------
vtkMultiOut3::~vtkMultiOut3()
{

}

//------------------------------------------------------------------------------
void vtkMultiOut3::ExecuteInformation()
{
  vtkImageData *image;

  image = static_cast<vtkImageData*>(this->GetOutput(0));
  image->SetWholeExtent(0, 120, 0, 250, 0, 250);
  
  image = static_cast<vtkImageData*>(this->GetOutput(1));
  image->SetWholeExtent(130, 250, 0, 250, 0, 250);
}

//------------------------------------------------------------------------------
void vtkMultiOut3::Execute()
{
  vtkImageData *image;

  vtkImageMandelbrotSource *mSource;

  mSource = vtkImageMandelbrotSource::New();
  mSource->SetMaximumNumberOfIterations(20);
  
  mSource->SetWholeExtent(0, 120, 0, 250, 0, 100);
  mSource->SetSizeCX(1.25, 2.5, 2.0, 1.5);
  mSource->Update();
  image = static_cast<vtkImageData*>(this->GetOutput(0));
  image->ShallowCopy(mSource->GetOutput());
  
  // Now name the first output.
  vtkCharArray *nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
  char *str = nameArray->WritePointer(0, 20);
  sprintf(str, "Mandelbrot left");
  image->GetFieldData()->AddArray(nameArray);
  nameArray->Delete();

  mSource->SetWholeExtent(130, 250, 0, 250, 0, 100);
  mSource->SetSizeCX(1.25, 2.5, 2.0, 1.5);
  mSource->Update();
  image = static_cast<vtkImageData*>(this->GetOutput(1));
  image->ShallowCopy(mSource->GetOutput());

  // Now name the second output.
  nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
  str = nameArray->WritePointer(0, 20);
  sprintf(str, "Mandelbrot Right");
  image->GetFieldData()->AddArray(nameArray);
  nameArray->Delete();

  mSource->Delete();
}

//------------------------------------------------------------------------------
void vtkMultiOut3::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

