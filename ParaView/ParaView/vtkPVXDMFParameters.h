/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVXDMFParameters.h
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
// .NAME vtkPVXDMFParameters -
// .SECTION Description

#ifndef __vtkPVXDMFParameters_h
#define __vtkPVXDMFParameters_h

#include "vtkPVObjectWidget.h"

class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWScale;
class vtkPVXDMFParametersInternals;

class VTK_EXPORT vtkPVXDMFParameters : public vtkPVObjectWidget
{
public:
  static vtkPVXDMFParameters* New();
  vtkTypeRevisionMacro(vtkPVXDMFParameters, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // Check if the widget was modified.
  void CheckModifiedCallback();
  
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVXDMFParameters* ClonePrototype(vtkPVSource* pvSource,
                             vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Side effect is to turn modified flag off.
  virtual void AcceptInternal(vtkClientServerID);
  
  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // The label.
  void SetLabel(const char* label);

  // Description:
  // This method updates values from the reader
  void UpdateFromReader();

  // Description:
  // This method adds parameter with value and range to the list.
  void AddXDMFParameter(const char* pname, int value, int min, int step, int max);

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // This method gets called when the user selects this widget to animate.
  // It sets up the script and animation parameters.
  void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai, const char* name);


  // Description:
  // Get the frame
  vtkGetObjectMacro(Frame, vtkKWLabeledFrame);
  vtkGetStringMacro(VTKReaderTclName);

  void SaveInBatchScript(ofstream *file);

protected:
  vtkPVXDMFParameters();
  ~vtkPVXDMFParameters();
  

  vtkPVXDMFParametersInternals* Internals;
  vtkKWLabeledFrame* Frame;


//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  vtkSetStringMacro(FrameLabel);
  vtkGetStringMacro(FrameLabel);
  char* FrameLabel;

  // Description:
  // This is the name of the VTK reader.
  vtkSetStringMacro(VTKReaderTclName);
  char* VTKReaderTclName;

private:
  vtkPVXDMFParameters(const vtkPVXDMFParameters&); // Not implemented
  void operator=(const vtkPVXDMFParameters&); // Not implemented
};

#endif
