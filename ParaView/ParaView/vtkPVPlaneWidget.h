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
// I could not descide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVPlaneWidget_h
#define __vtkPVPlaneWidget_h

#include "vtkPV3DWidget.h"

class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;

class VTK_EXPORT vtkPVPlaneWidget : public vtkPV3DWidget
{
public:
  static vtkPVPlaneWidget* New();
  vtkTypeMacro(vtkPVPlaneWidget, vtkPV3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
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
  // For saving the widget into a VTK tcl script.
  void SaveInTclScript(ofstream *file);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVPlaneWidget* ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  void SetCenter();
  void SetCenter(float,float,float);
  void SetNormal();
  void SetNormal(float,float,float);

protected:
  vtkPVPlaneWidget();
  ~vtkPVPlaneWidget();

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  vtkKWEntry *CenterEntry[3];
  vtkKWPushButton *CenterResetButton;

  vtkKWEntry *NormalEntry[3];

  vtkKWWidget *NormalButtonFrame;
  vtkKWPushButton *NormalCameraButton;
  vtkKWPushButton *NormalXButton;
  vtkKWPushButton *NormalYButton;
  vtkKWPushButton *NormalZButton;
  vtkKWLabel* Labels[2];
  vtkKWLabel* CoordinateLabel[3];

  char *PlaneTclName;
  vtkSetStringMacro(PlaneTclName);


  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVPlaneWidget(const vtkPVPlaneWidget&); // Not implemented
  void operator=(const vtkPVPlaneWidget&); // Not implemented
};

#endif
