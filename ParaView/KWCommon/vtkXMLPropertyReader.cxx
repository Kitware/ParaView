/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPropertyReader.cxx
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
#include "vtkXMLPropertyReader.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLPropertyWriter.h"

vtkStandardNewMacro(vtkXMLPropertyReader);
vtkCxxRevisionMacro(vtkXMLPropertyReader, "1.1");

vtkCxxSetObjectMacro(vtkXMLPropertyReader, Property, vtkProperty);

//----------------------------------------------------------------------------
vtkXMLPropertyReader::vtkXMLPropertyReader() 
{ 
  this->Property = 0;
} 

//----------------------------------------------------------------------------
vtkXMLPropertyReader::~vtkXMLPropertyReader() 
{ 
  this->SetProperty(0);
}

//----------------------------------------------------------------------------
void vtkXMLPropertyReader::StartElement(const char *name, const char **args)
{
  if (!this->Property)
    {
    vtkWarningMacro(<< "The Property is not set!");
    return;
    }

  if (strcmp(name, "Property"))
    {
    return;
    }

  float fbuffer3[3];
  int i;

  for (i = 0; args[i] && args[i + 1]; i += 2)
    {
    if (!strcmp(args[i], "Interpolation"))
      {
      this->Property->SetInterpolation(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "Representation"))
      {
      this->Property->SetRepresentation(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "Color"))
      {
      sscanf(args[i + 1], "%f %f %f", fbuffer3, fbuffer3 + 1, fbuffer3 + 2);
      this->Property->SetColor(fbuffer3);
      }
    else if (!strcmp(args[i], "Ambient"))
      {
      this->Property->SetAmbient(atof(args[i + 1]));
      }
    else if (!strcmp(args[i], "Diffuse"))
      {
      this->Property->SetDiffuse(atof(args[i + 1]));
      }
    else if (!strcmp(args[i], "Specular"))
      {
      this->Property->SetSpecular(atof(args[i + 1]));
      }
    else if (!strcmp(args[i], "SpecularPower"))
      {
      this->Property->SetSpecularPower(atof(args[i + 1]));
      }
    else if (!strcmp(args[i], "Opacity"))
      {
      this->Property->SetOpacity(atof(args[i + 1]));
      }
    else if (!strcmp(args[i], "AmbientColor"))
      {
      sscanf(args[i + 1], "%f %f %f", fbuffer3, fbuffer3 + 1, fbuffer3 + 2);
      this->Property->SetAmbientColor(fbuffer3);
      }
    else if (!strcmp(args[i], "DiffuseColor"))
      {
      sscanf(args[i + 1], "%f %f %f", fbuffer3, fbuffer3 + 1, fbuffer3 + 2);
      this->Property->SetDiffuseColor(fbuffer3);
      }
    else if (!strcmp(args[i], "SpecularColor"))
      {
      sscanf(args[i + 1], "%f %f %f", fbuffer3, fbuffer3 + 1, fbuffer3 + 2);
      this->Property->SetSpecularColor(fbuffer3);
      }
    else if (!strcmp(args[i], "EdgeVisibility"))
      {
      this->Property->SetEdgeVisibility(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "EdgeColor"))
      {
      sscanf(args[i + 1], "%f %f %f", fbuffer3, fbuffer3 + 1, fbuffer3 + 2);
      this->Property->SetEdgeColor(fbuffer3);
      }
    else if (!strcmp(args[i], "LineWidth"))
      {
      this->Property->SetLineWidth(atof(args[i + 1]));
      }
    else if (!strcmp(args[i], "LineStipplePattern"))
      {
      this->Property->SetLineStipplePattern(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "LineStippleRepeatFactor"))
      {
      this->Property->SetLineStippleRepeatFactor(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "PointSize"))
      {
      this->Property->SetPointSize(atof(args[i + 1]));
      }
    else if (!strcmp(args[i], "BackfaceCulling"))
      {
      this->Property->SetBackfaceCulling(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "FrontfaceCulling"))
      {
      this->Property->SetFrontfaceCulling(atoi(args[i + 1]));
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLPropertyReader::PrintSelf(ostream& os, vtkIndent indent)
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
