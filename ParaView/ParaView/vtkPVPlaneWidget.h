/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlaneWidget.h
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
// .NAME vtkPVPlaneWidget - A widget to manipulate an implicit plane.
// .SECTION Description
// This widget creates and manages its own vtkPlane on each process.
// I could not descide whether to include the bounds display or not. (I did not.) 


#ifndef __vtkPVPlaneWidget_h
#define __vtkPVPlaneWidget_h

#include "vtkPVWidget.h"
class vtkPVSource;


class VTK_EXPORT vtkPVPlaneWidget : public vtkPVWidget
{
public:
  static vtkPVPlaneWidget* New();
  vtkTypeMacro(vtkPVPlaneWidget, vtkPVWidget);
    
  virtual void Create(vtkKWApplication *app);

  // Description:
  // This widget needs access to the PVSource for reseting the center to
  // the middle of the data bounds and getting the camera for setting
  // the normal to the view plane normal.  If this widget gets used for some 
  // other purpose, then we might consider changing this reference to a 
  // render view and data object.
  // Note: There is no reference counting for fear of reference loops.
  // The pvSource owns this widget.
  void SetPVSource(vtkPVSource *pvs) {this->PVSource = pvs;}
  vtkPVSource *GetPVSource() {return this->PVSource;}

  // Description:
  // Callback that set the center to the middle of the bounds.
  void CenterResetCallback();

  // Descript:
  // Callbacks to set the normal.
  void NormalCameraCallback();
  void NormalXCallback();
  void NormalYCallback();
  void NormalZCallback();

  // Description:
  // Called when the PVSources reset button is called.
  virtual void Reset();
    
  // Description:
  // Called when the PVSources accept button is called.
  // It can also puts an entry in the trace file.
  virtual void Accept();

  // Description:
  // The Tcl name of the VTK implicit plane.
  vtkGetStringMacro(PlaneTclName);

  // Description:
  // Access to the widgets is required for tracing and scripting.
  vtkGetObjectMacro(CenterEntry, vtkPVVectorEntry);
  vtkGetObjectMacro(NormalEntry, vtkPVVectorEntry);

protected:
  vtkPVPlaneWidget();
  ~vtkPVPlaneWidget();
  vtkPVPlaneWidget(const vtkPVPlaneWidget&) {};
  void operator=(const vtkPVPlaneWidget&) {};

  vtkPVSource *PVSource;

  vtkPVVectorEntry *CenterEntry;
  vtkKWPushButton *CenterResetButton;

  vtkPVVectorEntry *NormalEntry;

  vtkKWWidget *NormalButtonFrame;
  vtkKWPushButton *NormalCameraButton;
  vtkKWPushButton *NormalXButton;
  vtkKWPushButton *NormalYButton;
  vtkKWPushButton *NormalZButton;

  char *PlaneTclName;
  vtkSetStringMacro(PlaneTclName);
};

#endif
