/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCollectionWriter.cxx
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
#include "vtkPVCollectionWriter.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCollectionWriter);
vtkCxxRevisionMacro(vtkPVCollectionWriter, "1.3");

//----------------------------------------------------------------------------
vtkPVCollectionWriter::vtkPVCollectionWriter()
{
}

//----------------------------------------------------------------------------
vtkPVCollectionWriter::~vtkPVCollectionWriter()
{
}

//----------------------------------------------------------------------------
void vtkPVCollectionWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVCollectionWriter::CanWriteData(vtkDataSet* data, int, int)
{
  // We support all data types in both parallel and serial mode, and
  // with any number of parts.
  return data?1:0;
}

//----------------------------------------------------------------------------
void vtkPVCollectionWriter::Write(const char* fileName, vtkPVSource* pvs,
                                  int numProcs, int ghostLevel)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  
  int i;
  pm->ServerScript(
    "vtkXMLPVCollectionWriter writer\n"
    "writer SetNumberOfPieces %d\n"
    "writer SetGhostLevel %d\n"
    "writer SetFileName {%s}",
    numProcs, ghostLevel, fileName);
  for(i=0; i < pvs->GetNumberOfPVParts(); ++i)
    {
    pm->ServerScript("writer AddInput \"%s\"",
                     pvs->GetPVPart(i)->GetVTKDataTclName());
    }
  pm->RootScript("writer SetPiece 0");
  for (i=1; i < numProcs; ++i)
    {
    pvApp->RemoteScript(i, "writer SetPiece %d", i);
    }
  pm->ServerScript("writer Write");
  pm->ServerScript("writer Delete");
}
