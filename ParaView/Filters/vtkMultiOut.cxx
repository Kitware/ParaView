/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiOut.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiOut.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkFieldData.h"
#include "vtkSphereSource.h"
#include "vtkCharArray.h"

#include <math.h>

vtkCxxRevisionMacro(vtkMultiOut, "1.2");
vtkStandardNewMacro(vtkMultiOut);

//------------------------------------------------------------------------------
vtkMultiOut::vtkMultiOut()
{
  int idx;
  vtkPolyData *pd;

  this->NumberOfSpheres = 100;
  this->SetNumberOfOutputs(this->NumberOfSpheres);
  for (idx = 0; idx < this->NumberOfSpheres; ++idx)
    {
    pd = vtkPolyData::New();
    this->SetNthOutput(idx, pd);
    pd->Delete();
    }
}

//------------------------------------------------------------------------------
vtkMultiOut::~vtkMultiOut()
{

}


//------------------------------------------------------------------------------
void vtkMultiOut::Execute()
{
  vtkPolyData *output;
  int idx;
  vtkSphereSource *sphere = vtkSphereSource::New();
  float rad;
  float cx, cy, cz;
  int count = 10;

  cx = cy = cz = 0.0;

  sphere->SetRadius(0.5);
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(12);
  rad = 1.0;
  cx = -rad;
  for (idx = 0; idx < this->NumberOfSpheres; ++idx)
    {
    // Move center to surface of last sphere.
    cx += rad;
    // Compute new radius.
    rad *= 0.99;
    // Move center over so new sphere touches old sphere.
    cx += rad;

    if (++count >= 10)
      {
      count = 0;
      cx = 0.0;
      cy += 2.0;
      }

    output = static_cast<vtkPolyData*>(this->GetOutput(idx));
    sphere->SetCenter(cx, cy, cz);
    sphere->SetRadius(rad);
    sphere->Update();
    output->ShallowCopy(sphere->GetOutput());
    vtkCharArray *nameArray = vtkCharArray::New();
    nameArray->SetName("Name");
    char *str = nameArray->WritePointer(0, 20);
    sprintf(str, "Sphere %d", idx);
    output->GetFieldData()->AddArray(nameArray);
    nameArray->Delete();
    }

  sphere->Delete();
}

//------------------------------------------------------------------------------
void vtkMultiOut::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfOutputs: " << this->NumberOfSpheres;
}

