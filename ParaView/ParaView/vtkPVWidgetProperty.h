/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWidgetProperty.h
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
// .NAME vtkPVWidgetProperty - a class for passing values to a VTK object
// .SECTION Description
// vtkPVWidgetProperty is a class for passing values from a GUI element to a
// VTK object.

#ifndef __vtkPVWidgetProperty_h
#define __vtkPVWidgetProperty_h

#include "vtkObject.h"

class vtkPVWidget;

class VTK_EXPORT vtkPVWidgetProperty : public vtkObject
{
public:
  static vtkPVWidgetProperty* New();
  vtkTypeRevisionMacro(vtkPVWidgetProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the vtkPVWidget that this property holds values for.
  void SetWidget(vtkPVWidget *widget);
  vtkGetObjectMacro(Widget, vtkPVWidget);

  // Description:
  // Call Reset on the vtkPVWidget this object has a pointer to.
  // This causes the values in the widget to be set to the ones in this
  // property.
  virtual void Reset();
  
  // Description:
  // Call Accept on the vtkPVWidget this object has a pointer to.
  // This causes the values in this property to be set to the ones in the
  // widget.
  virtual void Accept();
  
  // Description:
  // Pass the values from this property to VTK.
  virtual void AcceptInternal() {}

  // Description:
  // Set the Tcl name of the VTK object that values will be passed to.
  vtkSetStringMacro(VTKSourceTclName);

  // Description:
  // Set the animation time for this property.  This sets the modified flag on
  // the widget, and then calls Reset on it.
  virtual void SetAnimationTime(float) {}
  
protected:
  vtkPVWidgetProperty();
  ~vtkPVWidgetProperty();

  vtkPVWidget *Widget;
  char *VTKSourceTclName;
  
private:
  vtkPVWidgetProperty(const vtkPVWidgetProperty&); // Not implemented
  void operator=(const vtkPVWidgetProperty&); // Not implemented
};

#endif
