/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtentEntry.h
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
// .NAME vtkPVExtentEntry -
// .SECTION Description
// Although I could make this a subclass of vtkPVVector Entry,
// Vector entry is too general, and some inherited method may be confusing.
// The resaon I created this class is to get a special behavior
// for animations.

#ifndef __vtkPVExtentEntry_h
#define __vtkPVExtentEntry_h

#include "vtkPVObjectWidget.h"

class vtkKWEntry;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkPVInputMenu;
class vtkPVMinMax;
class vtkPVScalarListWidgetProperty;

class VTK_EXPORT vtkPVExtentEntry : public vtkPVObjectWidget
{
public:
  static vtkPVExtentEntry* New();
  vtkTypeRevisionMacro(vtkPVExtentEntry, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // Methods to set this widgets value from a script.
  void SetValue(int v1, int v2, int v3, int v4, int v5, int v6);
  
  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // This method gets called when the user selects this widget to animate.
  // It sets up the script and animation parameters.
  void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai, int Mode);

  // Description:
  // The label.
  vtkSetStringMacro(Label);
  vtkGetStringMacro(Label);

  // Description:
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);

  // Description:
  virtual void Update();

  // Description:
  // The label.
  void SetRange(int v0, int v1, int v2, int v3, int v4, int v5);
  vtkGetVector6Macro(Range,int);

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
  vtkPVExtentEntry* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  // Description:
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Side effect is to turn modified flag off.
  virtual void AcceptInternal(const char* sourceTclName);
  
  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
protected:
  vtkPVExtentEntry();
  ~vtkPVExtentEntry();

  vtkKWLabeledFrame* LabeledFrame;
  char* Label;

  vtkPVInputMenu* InputMenu;

  int Range[6];
  vtkPVMinMax* MinMax[3];

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  int AcceptCalled;

  vtkPVScalarListWidgetProperty *Property;
  
private:
  vtkPVExtentEntry(const vtkPVExtentEntry&); // Not implemented
  void operator=(const vtkPVExtentEntry&); // Not implemented
};

#endif
