/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLCameraReader.cxx
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
#include "vtkXMLCameraReader.h"

#include "vtkObjectFactory.h"
#include "vtkCamera.h"

vtkStandardNewMacro(vtkXMLCameraReader);
vtkCxxRevisionMacro(vtkXMLCameraReader, "1.2");

vtkCxxSetObjectMacro(vtkXMLCameraReader, Camera, vtkCamera);

//----------------------------------------------------------------------------
vtkXMLCameraReader::vtkXMLCameraReader() 
{ 
  this->Camera = 0;
} 

//----------------------------------------------------------------------------
vtkXMLCameraReader::~vtkXMLCameraReader() 
{ 
  this->SetCamera(0);
}

//----------------------------------------------------------------------------
void vtkXMLCameraReader::StartElement(const char *name, const char **args)
{
  if (!this->Camera || strcmp(name, "Camera"))
    {
    return;
    }

  double dbuffer3[3];
  int i;

  for (i = 0; args[i] && args[i + 1]; i += 2)
    {
    if (!strcmp(args[i], "ParallelProjection"))
      {
      this->Camera->SetParallelProjection(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "Position"))
      {
      sscanf(args[i + 1], "%lf %lf %lf", dbuffer3, dbuffer3 + 1, dbuffer3 + 2);
      this->Camera->SetPosition(dbuffer3);
      }
    else if (!strcmp(args[i], "FocalPoint"))
      {
      sscanf(args[i + 1], "%lf %lf %lf", dbuffer3, dbuffer3 + 1, dbuffer3 + 2);
      this->Camera->SetFocalPoint(dbuffer3);
      }
    else if (!strcmp(args[i], "ViewUp"))
      {
      sscanf(args[i + 1], "%lf %lf %lf", dbuffer3, dbuffer3 + 1, dbuffer3 + 2);
      this->Camera->SetViewUp(dbuffer3);
      }
    else if (!strcmp(args[i], "ClippingRange"))
      {
      sscanf(args[i + 1], "%lf %lf %lf", dbuffer3, dbuffer3 + 1, dbuffer3 + 2);
      this->Camera->SetClippingRange(dbuffer3);
      }
    else if (!strcmp(args[i], "ViewAngle"))
      {
      this->Camera->SetViewAngle(atof(args[i + 1]));
      }
    else if (!strcmp(args[i], "ParallelScale"))
      {
      this->Camera->SetParallelScale(atof(args[i + 1]));
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLCameraReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->Camera)
    {
    os << indent << "Camera: " << this->Camera << "\n";
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }
}
