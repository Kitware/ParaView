/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLTextPropertyReader.cxx
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
#include "vtkXMLTextPropertyReader.h"

#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"

vtkStandardNewMacro(vtkXMLTextPropertyReader);
vtkCxxRevisionMacro(vtkXMLTextPropertyReader, "1.2");

vtkCxxSetObjectMacro(vtkXMLTextPropertyReader, TextProperty, vtkTextProperty);

//----------------------------------------------------------------------------
vtkXMLTextPropertyReader::vtkXMLTextPropertyReader() 
{ 
  this->TextProperty = 0;
} 

//----------------------------------------------------------------------------
vtkXMLTextPropertyReader::~vtkXMLTextPropertyReader() 
{ 
  this->SetTextProperty(0);
}

//----------------------------------------------------------------------------
void vtkXMLTextPropertyReader::StartElement(const char *name, const char **args)
{
  if (!this->TextProperty)
    {
    vtkWarningMacro(<< "The TextProperty is not set!");
    return;
    }

  if (strcmp(name, "TextProperty"))
    {
    return;
    }

  float fbuffer3[3];
  int i;

  for (i = 0; args[i] && args[i + 1]; i += 2)
    {
    if (!strcmp(args[i], "Color"))
      {
      sscanf(args[i + 1], "%f %f %f", fbuffer3, fbuffer3 + 1, fbuffer3 + 2);
      this->TextProperty->SetColor(fbuffer3);
      }
    else if (!strcmp(args[i], "Opacity"))
      {
      this->TextProperty->SetOpacity(atof(args[i + 1]));
      }
    else if (!strcmp(args[i], "FontFamily"))
      {
      this->TextProperty->SetFontFamily(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "FontSize"))
      {
      this->TextProperty->SetFontSize(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "Bold"))
      {
      this->TextProperty->SetBold(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "Italic"))
      {
      this->TextProperty->SetItalic(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "Shadow"))
      {
      this->TextProperty->SetShadow(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "AntiAliasing"))
      {
      this->TextProperty->SetAntiAliasing(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "Justification"))
      {
      this->TextProperty->SetJustification(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "VerticalJustification"))
      {
      this->TextProperty->SetVerticalJustification(atoi(args[i + 1]));
      }
    else if (!strcmp(args[i], "LineOffset"))
      {
      this->TextProperty->SetLineOffset(atof(args[i + 1]));
      }
    else if (!strcmp(args[i], "LineSpacing"))
      {
      this->TextProperty->SetLineSpacing(atof(args[i + 1]));
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLTextPropertyReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->TextProperty)
    {
    os << indent << "TextProperty: " << this->TextProperty << "\n";
    }
  else
    {
    os << indent << "TextProperty: (none)\n";
    }
}
