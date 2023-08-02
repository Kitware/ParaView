// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
