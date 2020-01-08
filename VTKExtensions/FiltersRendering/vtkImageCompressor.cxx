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

#include "vtkCommand.h"
#include "vtkMultiProcessStream.h"
#include "vtkUnsignedCharArray.h"
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkImageCompressor, Output, vtkUnsignedCharArray);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkImageCompressor, Input, vtkUnsignedCharArray);

//-----------------------------------------------------------------------------
vtkImageCompressor::vtkImageCompressor()
  : Output(0)
  , Input(0)
  , LossLessMode(0)
  , Configuration(0)
{
  // Always allocate output array as a convenience.
  vtkUnsignedCharArray* data = vtkUnsignedCharArray::New();
  this->SetOutput(data);
  data->Delete();
}

//-----------------------------------------------------------------------------
vtkImageCompressor::~vtkImageCompressor()
{
  this->SetOutput(0);
  this->SetInput(0);
  this->SetConfiguration(NULL);
}

//-----------------------------------------------------------------------------
void vtkImageCompressor::SetImageResolution(int, int)
{
}

//-----------------------------------------------------------------------------
void vtkImageCompressor::SaveConfiguration(vtkMultiProcessStream* stream)
{
  *stream << this->GetClassName() << this->GetLossLessMode();
}

//-----------------------------------------------------------------------------
const char* vtkImageCompressor::SaveConfiguration()
{
  std::ostringstream oss;
  oss << this->GetClassName() << " " << this->GetLossLessMode();

  this->SetConfiguration(oss.str().c_str());

  return this->Configuration;
}

//-----------------------------------------------------------------------------
bool vtkImageCompressor::RestoreConfiguration(vtkMultiProcessStream* stream)
{
  std::string typeStr;
  *stream >> typeStr;
  if (typeStr == this->GetClassName())
  {
    int mode;
    *stream >> mode;
    this->SetLossLessMode(mode);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
const char* vtkImageCompressor::RestoreConfiguration(const char* stream)
{
  std::istringstream iss(stream);
  std::string typeStr;
  iss >> typeStr;
  if (typeStr == this->GetClassName())
  {
    int mode;
    iss >> mode;
    this->SetLossLessMode(mode);
    return stream + iss.tellg();
  }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkImageCompressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Input:          " << this->Input << endl
     << indent << "Output:         " << this->Output << endl
     << indent << "LossLessMode: " << this->LossLessMode << endl;
}
