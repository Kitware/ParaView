/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContourEntry.h
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
// .NAME vtkPVContourEntry maintains a list of floats for contouring.
// .SECTION Description
// This widget lets the user add or delete floats from a list.
// It is used for contours, but could be generalized for arbitrary lists.

#ifndef __vtkPVContourEntry_h
#define __vtkPVContourEntry_h

#include "vtkPVWidget.h"
#include "vtkKWLabel.h"

class vtkKWListBox;
class vtkKWEntry;
class vtkKWPushButton;

class VTK_EXPORT vtkPVContourEntry : public vtkPVWidget
{
public:
  static vtkPVContourEntry* New();
  vtkTypeMacro(vtkPVContourEntry, vtkPVWidget);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set the label.  The label can be used to get this widget
  // from a script.
  void SetLabel (const char* label);
  const char* GetLabel() {return this->ContourValuesLabel->GetLabel();}
  
  // Description:
  // Gets called when the accept button is pressed.
  // This method may add an entry to the trace file.
  virtual void Accept();

  // Description:
  // Gets called when the reset button is pressed.
  virtual void Reset();

  // Description:
  // Access to this widget from a script.
  void AddValue(char* val);
  void RemoveAllValues();

  // Description:
  // Button callbacks.
  void AddValueCallback();
  void DeleteValueCallback();

protected:
  vtkPVContourEntry();
  ~vtkPVContourEntry();
  vtkPVContourEntry(const vtkPVContourEntry&) {};
  void operator=(const vtkPVContourEntry&) {};

  vtkKWLabel* ContourValuesLabel;
  vtkKWListBox *ContourValuesList;
  vtkKWWidget* NewValueFrame;
  vtkKWLabel* NewValueLabel;
  vtkKWEntry* NewValueEntry;
  vtkKWPushButton* AddValueButton;
  vtkKWPushButton* DeleteValueButton;
};

#endif
