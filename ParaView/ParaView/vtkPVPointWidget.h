/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPointWidget.h
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
// .NAME vtkPVPointWidget - A widget to manipulate an implicit plane.
// .SECTION Description
// This widget creates and manages its own vtkPlane on each process.
// I could not descide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVPointWidget_h
#define __vtkPVPointWidget_h

#include "vtkPV3DWidget.h"

class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;

class VTK_EXPORT vtkPVPointWidget : public vtkPV3DWidget
{
public:
  static vtkPVPointWidget* New();
  vtkTypeRevisionMacro(vtkPVPointWidget, vtkPV3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Callback that set the center to the middle of the bounds.
  void PositionResetCallback();

  // Description:
  // This method sets the input to the 3D widget and places the widget.
  virtual void ActualPlaceWidget();

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVPointWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  void SetPosition();
  void SetPosition(float,float,float);
  void SetPositionInternal(float,float,float);
  void GetPosition(float pt[3]);

  // Description:
  // For saving the widget into a VTK tcl script.
  virtual void SaveInBatchScriptForPart(ofstream *file, vtkClientServerID);

  // Description:
  // Called when the PVSources reset button is called.
  virtual void ResetInternal();
    
  // Description:
  // Called when the PVSources accept button is called.
  virtual void AcceptInternal(vtkClientServerID);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

protected:
  vtkPVPointWidget();
  ~vtkPVPointWidget();


  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  void UpdateVTKObject();

  vtkKWEntry *PositionEntry[3];
  vtkKWPushButton *PositionResetButton;

  vtkKWLabel* Labels[2];
  vtkKWLabel* CoordinateLabel[3];

  float LastAcceptedPosition[3];
  vtkSetVector3Macro(LastAcceptedPosition, float);
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVPointWidget(const vtkPVPointWidget&); // Not implemented
  void operator=(const vtkPVPointWidget&); // Not implemented
};

#endif
