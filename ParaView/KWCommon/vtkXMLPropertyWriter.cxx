/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPropertyWriter.cxx
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
#include "vtkXMLPropertyWriter.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkXMLPropertyWriter);
vtkCxxRevisionMacro(vtkXMLPropertyWriter, "1.1");

vtkCxxSetObjectMacro(vtkXMLPropertyWriter, Property, vtkProperty);

//----------------------------------------------------------------------------
vtkXMLPropertyWriter::vtkXMLPropertyWriter()
{
  this->Property = 0;
}

//----------------------------------------------------------------------------
vtkXMLPropertyWriter::~vtkXMLPropertyWriter()
{
  this->SetProperty(0);
}

//----------------------------------------------------------------------------
int vtkXMLPropertyWriter::Write(ostream &os, vtkIndent vtkNotUsed(indent))
{
  if (!this->Property)
    {
    vtkWarningMacro(<< "The Property is not set!");
    return 0;
    }

  float *fptr;

  os << "<Property Version=\"$Revision: 1.1 $\"";

  os << " Interpolation=\"" << this->Property->GetInterpolation()<< "\"";

  os << " Representation=\"" << this->Property->GetRepresentation()<< "\"";

  fptr = this->Property->GetColor();
  if (fptr)
    {
    os << " Color=\"" << fptr[0] << " " << fptr[1] << " " << fptr[2] << "\"";
    }

  os << " Ambient=\"" << this->Property->GetAmbient()<< "\"";

  os << " Diffuse=\"" << this->Property->GetDiffuse()<< "\"";

  os << " Specular=\"" << this->Property->GetSpecular()<< "\"";

  os << " SpecularPower=\"" << this->Property->GetSpecularPower()<< "\"";

  os << " Opacity=\"" << this->Property->GetOpacity()<< "\"";

  fptr = this->Property->GetAmbientColor();
  if (fptr)
    {
    os << " AmbientColor=\"" 
       << fptr[0] << " " << fptr[1] << " " << fptr[2] << "\"";
    }

  fptr = this->Property->GetDiffuseColor();
  if (fptr)
    {
    os << " DiffuseColor=\"" 
       << fptr[0] << " " << fptr[1] << " " << fptr[2] << "\"";
    }

  fptr = this->Property->GetSpecularColor();
  if (fptr)
    {
    os << " SpecularColor=\"" 
       << fptr[0] << " " << fptr[1] << " " << fptr[2] << "\"";
    }

  os << " EdgeVisibility=\"" << this->Property->GetEdgeVisibility()<< "\"";

  fptr = this->Property->GetEdgeColor();
  if (fptr)
    {
    os << " EdgeColor=\"" 
       << fptr[0] << " " << fptr[1] << " " << fptr[2] << "\"";
    }

  os << " LineWidth=\"" << this->Property->GetLineWidth()<< "\"";

  os << " LineStipplePattern=\"" 
     << this->Property->GetLineStipplePattern()<< "\"";

  os << " LineStippleRepeatFactor=\"" 
     << this->Property->GetLineStippleRepeatFactor()<< "\"";

  os << " PointSize=\"" << this->Property->GetPointSize()<< "\"";

  os << " BackfaceCulling=\"" << this->Property->GetBackfaceCulling()<< "\"";

  os << " FrontfaceCulling=\"" << this->Property->GetFrontfaceCulling()<< "\"";

  os << "/>";

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPropertyWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->Property)
    {
    os << indent << "Property: " << this->Property << "\n";
    }
  else
    {
    os << indent << "Property: (none)\n";
    }
}
