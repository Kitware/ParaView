/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractPartsWidget.h
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
// .NAME vtkPVExtractPartsWidget - Widget for vtkSelectInputs filter.
// .SECTION Description
// This filter lets the user select a subset of parts fromthe input PVData.
// Since this widget will effect the outputs of the SelectInputsFilter,
// I will need to modify accept so the widgets accept methods get
// called before initialize data.
// I also have to add a widget option that fixes the widget value after
// accept is called.

#ifndef __vtkPVExtractPartsWidget_h
#define __vtkPVExtractPartsWidget_h

#include "vtkPVWidget.h"
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWListBox;
class vtkCollection;
class vtkPVScalarListWidgetProperty;

class VTK_EXPORT vtkPVExtractPartsWidget : public vtkPVWidget
{
public:
  static vtkPVExtractPartsWidget* New();
  vtkTypeRevisionMacro(vtkPVExtractPartsWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void Create(vtkKWApplication *app);

  // Description:
  // Save this source to a file.
  void SaveInBatchScript(ofstream *file);

  // Description:
  // Button callbacks.
  void AllOnCallback();
  void AllOffCallback();

  // Description:
  // Access metod necessary for scripting.
  void SetSelectState(int idx, int val);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Called when the Accept button is pressed.  It moves the widget values to the 
  // VTK calculator filter.
  virtual void AcceptInternal(const char* vtkSourceTclName);
  
  // Description:
  // This method resets the widget values from the VTK filter.
  virtual void ResetInternal();

  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
protected:
  vtkPVExtractPartsWidget();
  ~vtkPVExtractPartsWidget();

  vtkKWWidget* ButtonFrame;
  vtkKWPushButton* AllOnButton;
  vtkKWPushButton* AllOffButton;

  vtkKWListBox* PartSelectionList;
  // Labels get substituted for list box after accept is called.
  vtkCollection* PartLabelCollection;

  // Called to inactivate widget (after accept is called).
  void Inactivate();

  vtkPVScalarListWidgetProperty *Property;
  
  vtkPVExtractPartsWidget(const vtkPVExtractPartsWidget&); // Not implemented
  void operator=(const vtkPVExtractPartsWidget&); // Not implemented
};

#endif
