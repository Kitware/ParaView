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
#include "vtkXMLPiecewiseFunctionWriter.h"

#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPiecewiseFunctionWriter);
vtkCxxRevisionMacro(vtkXMLPiecewiseFunctionWriter, "1.2.4.1");

//----------------------------------------------------------------------------
char* vtkXMLPiecewiseFunctionWriter::GetRootElementName()
{
  return "PiecewiseFunction";
}

//----------------------------------------------------------------------------
char* vtkXMLPiecewiseFunctionWriter::GetPointElementName()
{
  return "Point";
}

//----------------------------------------------------------------------------
int vtkXMLPiecewiseFunctionWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkPiecewiseFunction *obj = vtkPiecewiseFunction::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PiecewiseFunction is not set!");
    return 0;
    }

  elem->SetIntAttribute("Size", obj->GetSize());

  elem->SetIntAttribute("Clamping", obj->GetClamping());

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPiecewiseFunctionWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkPiecewiseFunction *obj = vtkPiecewiseFunction::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PiecewiseFunction is not set!");
    return 0;
    }

  // Iterate over all points and create a point XML data element for each one

  int size = obj->GetSize();
  float *data_ptr = obj->GetDataPointer();

  if (size && data_ptr)
    {
    for (int i = 0; i < size; i++, data_ptr += 2)
      {
      vtkXMLDataElement *point_elem = this->NewDataElement();
      elem->AddNestedElement(point_elem);
      point_elem->Delete();
      point_elem->SetName(this->GetPointElementName());
      point_elem->SetFloatAttribute("X", data_ptr[0]);
      point_elem->SetFloatAttribute("Value", data_ptr[1]);
      }
    }

  return 1;
}


