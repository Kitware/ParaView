/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiOut2.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiOut2.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkSphereSource.h"
#include "vtkRTAnalyticSource.h"
#include "vtkCharArray.h"
#include "vtkFieldData.h"

#include <math.h>

vtkCxxRevisionMacro(vtkMultiOut2, "1.1");
vtkStandardNewMacro(vtkMultiOut2);

//------------------------------------------------------------------------------
vtkMultiOut2::vtkMultiOut2()
{
  vtkPolyData *pd;
  vtkImageData *image;

  this->SetNumberOfOutputs(2);

  pd = vtkPolyData::New();
  this->SetNthOutput(0, pd);
  pd->Delete();

  image = vtkImageData::New();
  this->SetNthOutput(1, image);
  image->Delete();

}

//------------------------------------------------------------------------------
vtkMultiOut2::~vtkMultiOut2()
{

}


//------------------------------------------------------------------------------
void vtkMultiOut2::Execute()
{
  vtkPolyData *pd;
  vtkImageData *image;

  vtkSphereSource *sphere = vtkSphereSource::New();
  sphere->SetRadius(10);
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(24);
  sphere->SetCenter(-15, 0, 0);
  sphere->Update();
  pd = static_cast<vtkPolyData*>(this->GetOutput(0));
  pd->ShallowCopy(sphere->GetOutput());
  sphere->Delete();

  // Now name the polydata (sphere).
  vtkCharArray *nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
  char *str = nameArray->WritePointer(0, 20);
  sprintf(str, "Sphere");
  pd->GetFieldData()->AddArray(nameArray);
  nameArray->Delete();

  // Add an extra array (RTData) so we can contour.
  vtkFloatArray *rtArray = vtkFloatArray::New();
  int idx, num;
  float *pt;
  num = pd->GetNumberOfPoints();
  for (idx = 0; idx < num; ++idx)
    {
    pt = pd->GetPoint(idx);
    rtArray->InsertNextValue(5.0*(pt[0]+pt[1]+pt[2]) + 150.0);
    }
  rtArray->SetName("RTData");
  pd->GetPointData()->SetScalars(rtArray);
  rtArray->Delete();

  vtkRTAnalyticSource *rtSource = vtkRTAnalyticSource::New();
  rtSource->Update();
  image = static_cast<vtkImageData*>(this->GetOutput(1));
  image->ShallowCopy(rtSource->GetOutput());
  rtSource->Delete();

  // Now name the image.
  nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
  str = nameArray->WritePointer(0, 20);
  sprintf(str, "Wavelet");
  image->GetFieldData()->AddArray(nameArray);
  nameArray->Delete();
}

//------------------------------------------------------------------------------
void vtkMultiOut2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

