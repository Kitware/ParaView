/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLCameraWriter.cxx
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
#include "vtkXMLCameraWriter.h"

#include "vtkObjectFactory.h"
#include "vtkCamera.h"

vtkStandardNewMacro(vtkXMLCameraWriter);
vtkCxxRevisionMacro(vtkXMLCameraWriter, "1.1");

vtkCxxSetObjectMacro(vtkXMLCameraWriter, Camera, vtkCamera);

//----------------------------------------------------------------------------
vtkXMLCameraWriter::vtkXMLCameraWriter()
{
  this->Camera = 0;
}

//----------------------------------------------------------------------------
vtkXMLCameraWriter::~vtkXMLCameraWriter()
{
  this->SetCamera(0);
}

//----------------------------------------------------------------------------
int vtkXMLCameraWriter::Write(ostream &os, vtkIndent vtkNotUsed(indent))
{
  if (!this->Camera)
    {
    return 0;
    }

  double *dptr;

  os << "<Camera Version=\"$Revision: 1.1 $\"";

  dptr = this->Camera->GetPosition();
  if (dptr)
    {
    os << " Position=\"" << dptr[0] << " " << dptr[1] << " " << dptr[2] << "\"";
    }

  dptr = this->Camera->GetFocalPoint();
  if (dptr)
    {
    os << " FocalPoint=\"" << dptr[0] << " " << dptr[1] << " " << dptr[2]<< "\"";
    }

  dptr = this->Camera->GetViewUp();
  if (dptr)
    {
    os << " ViewUp=\"" << dptr[0] << " " << dptr[1] << " " << dptr[2] << "\"";
    }

  dptr = this->Camera->GetClippingRange();
  if (dptr)
    {
    os << " ClippingRange=\"" 
       << dptr[0] << " " << dptr[1] << " " << dptr[2] << "\"";
    }

  os << " ViewAngle=\"" << this->Camera->GetViewAngle() << "\"";

  os << " ParallelScale=\"" << this->Camera->GetParallelScale() << "\"";

  os << " ParallelProjection=\"" << this->Camera->GetParallelProjection()<< "\"";

  os << "/>";

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLCameraWriter::PrintSelf(ostream& os, vtkIndent indent)
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
