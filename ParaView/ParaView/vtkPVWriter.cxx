/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVWriter.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVWriter);

//----------------------------------------------------------------------------
vtkPVWriter::vtkPVWriter()
{
  this->InputClassName = 0;
  this->WriterClassName = 0;
  this->Description = 0;
  this->Extension = 0;
  this->Parallel = 0;
}

//----------------------------------------------------------------------------
vtkPVWriter::~vtkPVWriter()
{
  this->SetInputClassName(0);
  this->SetWriterClassName(0);
  this->SetDescription(0);
  this->SetExtension(0);
}

//----------------------------------------------------------------------------
void vtkPVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputClassName: " 
     << (this->InputClassName?this->InputClassName:"null") << endl;
  os << indent << "WriterClassName: " 
     << (this->WriterClassName?this->WriterClassName:"null") << endl;
  os << indent << "Description: " 
     << (this->Description?this->Description:"null") << endl;
  os << indent << "Extension: " 
     << (this->Extension?this->Extension:"null") << endl;
  os << indent << "Parallel: " << this->Parallel << endl;
}

//----------------------------------------------------------------------------
int vtkPVWriter::CanWriteData(vtkDataSet* data, int parallel)
{
  return (parallel == this->Parallel) && data->IsA(this->InputClassName);
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVWriter::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->Application);
}

//----------------------------------------------------------------------------
void vtkPVWriter::Write(const char* fileName, const char* dataTclName,
                        int numProcs, int ghostLevel)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  if(!this->Parallel)
    {
    pvApp->BroadcastScript("%s writer", this->WriterClassName);
    pvApp->BroadcastScript("writer SetFileName %s", fileName);
    pvApp->BroadcastScript("writer SetInput %s", dataTclName);
    pvApp->BroadcastScript("writer Write");
    pvApp->BroadcastScript("writer Delete");
    }
  else
    {
    pvApp->BroadcastScript("%s writer", this->WriterClassName);
    pvApp->BroadcastScript("writer SetFileName %s", fileName);
    pvApp->BroadcastScript("writer SetInput %s", dataTclName);
    pvApp->BroadcastScript("writer SetNumberOfPieces %d", numProcs);
    pvApp->BroadcastScript("writer SetGhostLevel %d", ghostLevel);
    this->Script("writer SetStartPiece 0");
    this->Script("writer SetEndPiece 0");
    int i;
    for (i=1; i < numProcs; ++i)
      {
      pvApp->RemoteScript(i, "writer SetStartPiece %d", i);
      pvApp->RemoteScript(i, "writer SetEndPiece %d", i);
      }
    pvApp->BroadcastScript("writer Write");
    pvApp->BroadcastScript("writer Delete");
    }
}
