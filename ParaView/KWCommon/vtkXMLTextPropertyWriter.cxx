/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLTextPropertyWriter.cxx
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
#include "vtkXMLTextPropertyWriter.h"

#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"

vtkStandardNewMacro(vtkXMLTextPropertyWriter);
vtkCxxRevisionMacro(vtkXMLTextPropertyWriter, "1.1");

vtkCxxSetObjectMacro(vtkXMLTextPropertyWriter, TextProperty, vtkTextProperty);

//----------------------------------------------------------------------------
vtkXMLTextPropertyWriter::vtkXMLTextPropertyWriter()
{
  this->TextProperty = 0;
}

//----------------------------------------------------------------------------
vtkXMLTextPropertyWriter::~vtkXMLTextPropertyWriter()
{
  this->SetTextProperty(0);
}

//----------------------------------------------------------------------------
int vtkXMLTextPropertyWriter::Write(ostream &os, vtkIndent vtkNotUsed(indent))
{
  if (!this->TextProperty)
    {
    return 0;
    }

  float *fptr;

  os << "<TextProperty Version=\"$Revision: 1.1 $\"";

  fptr = this->TextProperty->GetColor();
  if (fptr)
    {
    os << " Color=\"" << fptr[0] << " " << fptr[1] << " " << fptr[2] << "\"";
    }

  os << " Opacity=\"" << this->TextProperty->GetOpacity() << "\"";

  os << " FontFamily=\"" << this->TextProperty->GetFontFamily() << "\"";

  os << " FontSize=\"" << this->TextProperty->GetFontSize() << "\"";

  os << " Bold=\"" << this->TextProperty->GetBold() << "\"";

  os << " Italic=\"" << this->TextProperty->GetItalic() << "\"";

  os << " Shadow=\"" << this->TextProperty->GetShadow() << "\"";

  os << " AntiAliasing=\"" << this->TextProperty->GetAntiAliasing() << "\"";

  os << " Justification=\"" << this->TextProperty->GetJustification() << "\"";

  os << " VerticalJustification=\"" 
     << this->TextProperty->GetVerticalJustification() << "\"";

  os << " LineOffset=\"" << this->TextProperty->GetLineOffset() << "\"";

  os << " LineSpacing=\"" << this->TextProperty->GetLineSpacing() << "\"";

  os << "/>";

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLTextPropertyWriter::PrintSelf(ostream& os, vtkIndent indent)
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
