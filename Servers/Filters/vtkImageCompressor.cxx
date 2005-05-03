/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCompressor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageCompressor.h"
#include "vtkObjectFactory.h"

#include "vtkUnsignedCharArray.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkImageCompressor, "1.1");
vtkCxxSetObjectMacro(vtkImageCompressor, Output, vtkUnsignedCharArray);
vtkCxxSetObjectMacro(vtkImageCompressor, Input, vtkUnsignedCharArray);
//-----------------------------------------------------------------------------
vtkImageCompressor::vtkImageCompressor()
{
  this->Output = 0;
  vtkUnsignedCharArray* data = vtkUnsignedCharArray::New();
  this->SetOutput(data);
  data->Delete();
  this->Input = 0;
}

//-----------------------------------------------------------------------------
vtkImageCompressor::~vtkImageCompressor()
{
  this->SetOutput(0);
  this->SetInput(0);
}

//-----------------------------------------------------------------------------
int vtkImageCompressor::Compress()
{
  // Make sure we have input.
  if (!this->Input)
    {
    vtkErrorMacro("No input provided!");
    return 0;
    }
  
  // always compress even if the data hasn;t changed.
  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  int ret = this->CompressData();
  this->InvokeEvent(vtkCommand::EndEvent,NULL);
  this->Modified();
  return ret;
}

//-----------------------------------------------------------------------------
int vtkImageCompressor::Decompress()
{
  // Make sure we have input.
  if (!this->Input)
    {
    vtkErrorMacro("No input provided!");
    return 0;
    }
  
  // always decompress even if the data hasn;t changed.
  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  int ret = this->DecompressData();
  this->InvokeEvent(vtkCommand::EndEvent,NULL);
  this->Modified();
  return ret;
}

//-----------------------------------------------------------------------------
void vtkImageCompressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

