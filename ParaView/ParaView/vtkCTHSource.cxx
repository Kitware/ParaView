/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHSource.cxx
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
#include "vtkCTHSource.h"
#include "vtkObjectFactory.h"
#include "vtkCTHData.h"


vtkCxxRevisionMacro(vtkCTHSource, "1.1.2.1");
vtkStandardNewMacro(vtkCTHSource);

//----------------------------------------------------------------------------
vtkCTHSource::vtkCTHSource()
{
  this->vtkSource::SetNthOutput(0, vtkCTHData::New());
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkCTHSource::~vtkCTHSource()
{
}

//----------------------------------------------------------------------------
vtkCTHData *vtkCTHSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkCTHData *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkCTHData *vtkCTHSource::GetOutput(int idx)
{
  return static_cast<vtkCTHData *>( this->vtkSource::GetOutput(idx) ); 
}

//----------------------------------------------------------------------------
void vtkCTHSource::SetOutput(vtkCTHData *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
void vtkCTHSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

