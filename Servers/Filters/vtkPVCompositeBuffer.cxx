/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeBuffer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVCompositeBuffer.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVCompositeBuffer, "1.4");
vtkStandardNewMacro(vtkPVCompositeBuffer);

//-------------------------------------------------------------------------
vtkPVCompositeBuffer::vtkPVCompositeBuffer()
{
  this->PData = NULL;
  this->ZData = NULL;
  this->UncompressedLength = -1;
}

  
//-------------------------------------------------------------------------
vtkUnsignedCharArray* vtkPVCompositeBuffer::GetPData()
{
  if (this->PData == NULL)
    {
    return NULL;
    }
  if (this->PData->GetNumberOfTuples() != this->UncompressedLength)
    {
    vtkErrorMacro("This buffer looks compressed.");
    }
  return this->PData;
}


//-------------------------------------------------------------------------
vtkPVCompositeBuffer::~vtkPVCompositeBuffer()
{
  if (this->PData)
    {
    this->PData->Delete();
    this->PData = NULL;
    }
  if (this->ZData)
    {
    this->ZData->Delete();
    this->ZData = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  
  os << indent << "PData: " << this->PData << endl;
  os << indent << "ZData: " << this->ZData << endl;
  os << indent << "UncompressedLength: " << this->UncompressedLength << endl;
}



