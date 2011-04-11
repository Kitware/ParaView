/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPriorityHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPriorityHelper.h"
#include "vtkObjectFactory.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPriorityHelper);

//-----------------------------------------------------------------------------
vtkPriorityHelper::vtkPriorityHelper()
{
  this->Input = NULL;
  this->Port = 0;
}

//-----------------------------------------------------------------------------
vtkPriorityHelper::~vtkPriorityHelper()
{
}

//-----------------------------------------------------------------------------
void vtkPriorityHelper::SetInputConnection(vtkAlgorithmOutput *port)
{
  this->Input = NULL;
  if (port && port->GetProducer())
    {
    this->Input = port->GetProducer();
    }
}

//-----------------------------------------------------------------------------
int vtkPriorityHelper::SetSplitUpdateExtent
  (int port,
   int processor,
   int numProcessors,
   int pass,
   int numPasses,
   double resolution
   )
{
  this->Port = port;
  //set currently active piece, remember settings to reuse internally
  if (this->Input)
    {
    vtkStreamingDemandDrivenPipeline *sddp =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(
        this->Input->GetExecutive());
    if (sddp)
      {
      sddp->SetUpdateExtent(this->Port,
                            processor*numProcessors + pass,
                            numProcessors*numPasses,
                            0); //ghost level
      sddp->SetUpdateResolution(this->Port, resolution);
      return 1;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataObject *vtkPriorityHelper::GetDataObject()
{
  if (this->Input)
    {
    return this->Input->GetOutputDataObject(this->Port);
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkPriorityHelper::Update()
{
  if (this->Input)
    {
    this->Input->Update();
    }
}
