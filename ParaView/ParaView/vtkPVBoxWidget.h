/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBoxWidget.h
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
// .NAME vtkPVBoxWidget - A widget to manipulate an box.
// .SECTION Description
// This widget creates and manages its own vtkPlanes on each process.
// I could not descide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVBoxWidget_h
#define __vtkPVBoxWidget_h

#include "vtkPV3DWidget.h"

class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;
class vtkKWThumbWheel;
class vtkKWScale;
class vtkTransform;
class vtkBoxWidget;

class VTK_EXPORT vtkPVBoxWidget : public vtkPV3DWidget
{
public:
  static vtkPVBoxWidget* New();
  vtkTypeRevisionMacro(vtkPVBoxWidget, vtkPV3DWidget);

  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // The Tcl name of the VTK implicit plane.
  vtkGetMacro(BoxID,vtkClientServerID);

  // Description:
  // The Tcl name of the VTK transform.
  vtkGetMacro(BoxTransformID,vtkClientServerID);
  //ETX

  // Description:
  // This method sets the input to the 3D widget and places the widget.
  virtual void ActualPlaceWidget();

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVBoxWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Called when the PVSources reset button is called.
  virtual void ResetInternal();

  //BTX
  // Description:
  // Called when the PVSources accept button is called.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // These are callbacks for the transformation of the box.
  void ScaleCallback();
  void TranslateCallback();
  void OrientationCallback();
  void ScaleEndCallback();
  void TranslateEndCallback();
  void OrientationEndCallback();

  // Description:
  // Set the box
  void SetScaleNoTrace(float p[3])
    { this->SetScaleNoTrace(p[0], p[1], p[2]); }
  void SetScaleNoTrace(float px, float py, float pz);
  void SetScale(float p[3])
    { this->SetScale(p[0], p[1], p[2]); }
  void SetScale(float px, float py, float pz);
  void SetTranslateNoTrace(float p[3])
    { this->SetTranslateNoTrace(p[0], p[1], p[2]); }
  void SetTranslateNoTrace(float px, float py, float pz);
  void SetTranslate(float p[3])
    { this->SetTranslate(p[0], p[1], p[2]); }
  void SetTranslate(float px, float py, float pz);
  void SetOrientationNoTrace(float p[3])
    { this->SetOrientationNoTrace(p[0], p[1], p[2]); }
  void SetOrientationNoTrace(float px, float py, float pz);
  void SetOrientation(float p[3])
    { this->SetOrientation(p[0], p[1], p[2]); }
  void SetOrientation(float px, float py, float pz);

  vtkBoxWidget* GetBoxWidget();

  vtkGetObjectMacro(BoxTransform, vtkTransform);

  // Description:
  // If update is specified, then it is updated from the gui, otherwise from
  // the stored variables.
  void UpdateBox(int update);
  void UpdateFromBox();

  // Description:
  // Return a contained object by name as follows:
  // Box == BoxID
  // BoxTransform == BoxTransformID
  // BoxMatrix == BoxMatrixID
  virtual vtkClientServerID GetObjectByName(const char*);


protected:
  vtkPVBoxWidget();
  ~vtkPVBoxWidget();

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  void UpdateVTKObject(const char* sourceTclName);

  vtkClientServerID BoxID;
  vtkClientServerID BoxTransformID;
  vtkClientServerID BoxMatrixID;

  vtkKWFrame*        ControlFrame;
  vtkKWLabel*        TranslateLabel;
  vtkKWThumbWheel*   TranslateThumbWheel[3];
  vtkKWLabel*        ScaleLabel;
  vtkKWThumbWheel*   ScaleThumbWheel[3];
  vtkKWLabel*        OrientationLabel;
  vtkKWScale*        OrientationScale[3];

  vtkTransform*      BoxTransform;

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  // Description:
  // For saving the widget into a VTK tcl script.
  // This saves the implicit sphere.  Parts will share this
  // one sphere.
  virtual void SaveInBatchScript(ofstream *file);

  float* GetPositionFromGUI();
  float* GetRotationFromGUI();
  float* GetScaleFromGUI();
  vtkSetVector3Macro(PositionGUI, float);
  vtkSetVector3Macro(RotationGUI, float);
  vtkSetVector3Macro(ScaleGUI,    float);
  float PositionGUI[3];
  float RotationGUI[3];
  float ScaleGUI[3];

  vtkSetVector3Macro(StoredPosition, float);
  vtkSetVector3Macro(StoredScale,    float);
  vtkSetVector3Macro(StoredRotation, float);
  float StoredPosition[3];
  float StoredRotation[3];
  float StoredScale[3];

  int Initialized;

private:
  vtkPVBoxWidget(const vtkPVBoxWidget&); // Not implemented
  void operator=(const vtkPVBoxWidget&); // Not implemented
};

#endif
