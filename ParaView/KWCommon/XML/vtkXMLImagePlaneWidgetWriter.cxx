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
#include "vtkXMLImagePlaneWidgetWriter.h"

#include "vtkImagePlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPropertyWriter.h"

vtkStandardNewMacro(vtkXMLImagePlaneWidgetWriter);
vtkCxxRevisionMacro(vtkXMLImagePlaneWidgetWriter, "1.1");

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetRootElementName()
{
  return "ImagePlaneWidget";
}

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetHandlePropertyElementName()
{
  return "HandleProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetSelectedHandlePropertyElementName()
{
  return "SelectedHandleProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetImagePlanePropertyElementName()
{
  return "ImagePlaneProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetSelectedImagePlanePropertyElementName()
{
  return "SelectedImagePlaneProperty";
}

//----------------------------------------------------------------------------
int vtkXMLImagePlaneWidgetWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkImagePlaneWidget *obj = vtkImagePlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ImagePlaneWidget is not set!");
    return 0;
    }

  elem->SetVectorAttribute("Origin", 3, obj->GetOrigin());

  elem->SetVectorAttribute("Point1", 3, obj->GetPoint1());

  elem->SetVectorAttribute("Point2", 3, obj->GetPoint2());

  return 1;
}
//----------------------------------------------------------------------------
int vtkXMLImagePlaneWidgetWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkImagePlaneWidget *obj = vtkImagePlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ImagePlaneWidget is not set!");
    return 0;
    }

  return 1;
}


