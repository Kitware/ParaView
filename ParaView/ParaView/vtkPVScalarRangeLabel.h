/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarRangeLabel.h
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
// .NAME vtkPVScalarRangeLabel - Shows the scalar range of and array.
// .SECTION Description
// This label gets an array from an array menu, and shows its scalar range.
// It shows nothing right now if the array has more than one component.


#ifndef __vtkPVScalarRangeLabel_h
#define __vtkPVScalarRangeLabel_h

#include "vtkPVWidget.h"
#include "vtkPVArrayMenu.h"
#include "vtkKWLabeledFrame.h"

class vtkKWApplication;

class VTK_EXPORT vtkPVScalarRangeLabel : public vtkPVWidget
{
public:
  static vtkPVScalarRangeLabel* New();
  vtkTypeMacro(vtkPVScalarRangeLabel, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app);

  // Description:
  // The scalar range display gets its array object from the array menu.
  // This is not reference counted because we want to avoid reference loops.
  void SetArrayMenu(vtkPVArrayMenu *aMenu) {this->ArrayMenu = aMenu;}
  vtkGetObjectMacro(ArrayMenu, vtkPVArrayMenu);

  // Description:
  // This calculates new range to display (using the array menu).
  virtual void Update();

  // Description:
  // Access to the range values.
  // This is used in a regression test.
  vtkGetVector2Macro(Range, float);

protected:
  vtkPVScalarRangeLabel();
  ~vtkPVScalarRangeLabel();

  vtkPVArrayMenu *ArrayMenu;
  vtkKWLabel *Label;

  float Range[2];

  vtkPVScalarRangeLabel(const vtkPVScalarRangeLabel&); // Not implemented
  void operator=(const vtkPVScalarRangeLabel&); // Not implemented
};


#endif
