/*=========================================================================

  Program:   ParaView
  Module:    vtkMyStringWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMyStringWriter.h"

#include <vtkObjectFactory.h>
#include <vtksys/FStream.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyStringWriter);

//----------------------------------------------------------------------------
bool vtkMyStringWriter::Write()
{
  vtksys::ofstream* fptr = new vtksys::ofstream(this->FileName.c_str(), ios::out);

  if (fptr->fail())
  {
    vtkErrorMacro(<< "Unable to open file: " << this->FileName);
    delete fptr;
    return false;
  }

  *fptr << this->Text;
  delete fptr;
  return true;
}

//----------------------------------------------------------------------------
void vtkMyStringWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "FileName: " << this->FileName << endl;
  os << indent << "Text: " << endl;
  os << indent << this->Text << endl;
  this->Superclass::PrintSelf(os, indent);
}
