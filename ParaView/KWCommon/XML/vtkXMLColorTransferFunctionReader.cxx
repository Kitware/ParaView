/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkXMLColorTransferFunctionReader.h"

#include "vtkColorTransferFunction.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLColorTransferFunctionWriter.h"

vtkStandardNewMacro(vtkXMLColorTransferFunctionReader);
vtkCxxRevisionMacro(vtkXMLColorTransferFunctionReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLColorTransferFunctionReader::GetRootElementName()
{
  return "ColorTransferFunction";
}

//----------------------------------------------------------------------------
int vtkXMLColorTransferFunctionReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkColorTransferFunction *obj = 
    vtkColorTransferFunction::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ColorTransferFunction is not set!");
    return 0;
    }

  // Get attributes

  int ival;

  if (elem->GetScalarAttribute("Clamping", ival))
    {
    obj->SetClamping(ival);
    }

  if (elem->GetScalarAttribute("ColorSpace", ival))
    {
    obj->SetColorSpace(ival);
    }

  // Get the points

  obj->RemoveAllPoints();

  int nb_nested_elems = elem->GetNumberOfNestedElements();
  for (int idx = 0; idx < nb_nested_elems; idx++)
    {
    vtkXMLDataElement *nested_elem = elem->GetNestedElement(idx);
    if (!strcmp(nested_elem->GetName(), 
                vtkXMLColorTransferFunctionWriter::GetPointElementName()))
      {
      float x, fbuffer3[3];
      if (nested_elem->GetScalarAttribute("X", x) &&
          nested_elem->GetVectorAttribute("Value", 3, fbuffer3) == 3)
        {
        obj->AddRGBPoint(x, fbuffer3[0], fbuffer3[1], fbuffer3[2]);
        }
      }
    }
  
  return 1;
}


