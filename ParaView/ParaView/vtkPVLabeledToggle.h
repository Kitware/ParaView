/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVLabeledToggle.h
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
// .NAME vtkPVLabeledToggle -
// .SECTION Description

#ifndef __vtkPVLabeledToggle_h
#define __vtkPVLabeledToggle_h

#include "vtkPVWidget.h"
#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"

class VTK_EXPORT vtkPVLabeledToggle : public vtkPVWidget
{
public:
  static vtkPVLabeledToggle* New();
  vtkTypeMacro(vtkPVLabeledToggle, vtkPVWidget);

  // Description:
  // Setting the label also sets the name.
  void SetLabel(const char *str) {this->Label->SetLabel(str); this->SetTraceName(str);}
  const char* GetLabel() { return this->Label->GetLabel();}

  void Create(vtkKWApplication *pvApp, char *help);
  
  // Description:
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Adds a trace entry.  Side effect is to turn modified flag off.
  virtual void Accept();
  
  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void Reset();
  
  // Description:
  // This method allows scripts to modify the widgets value.
  void SetState(int val);
  int GetState() { return this->CheckButton->GetState(); }

protected:
  vtkPVLabeledToggle();
  ~vtkPVLabeledToggle();
  vtkPVLabeledToggle(const vtkPVLabeledToggle&) {};
  void operator=(const vtkPVLabeledToggle&) {};
  
  vtkKWLabel *Label;
  vtkKWCheckButton *CheckButton;
};

#endif
