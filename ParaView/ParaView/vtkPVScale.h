/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScale.h
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
// .NAME vtkPVScale -
// .SECTION Description

#ifndef __vtkPVScale_h
#define __vtkPVScale_h

#include "vtkPVObjectWidget.h"

class vtkKWScale;
class vtkKWLabel;
class vtkPVScalarListWidgetProperty;

class VTK_EXPORT vtkPVScale : public vtkPVObjectWidget
{
public:
  static vtkPVScale* New();
  vtkTypeRevisionMacro(vtkPVScale, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // This method allows scripts to modify the widgets value.
  void SetValue(float val);
  float GetValue();

  // Description:
  // The label.
  void SetLabel(const char* label);

  // Description:
  // The resolution of the scale
  void SetResolution(float res);

  // Description:
  // Set the range of the scale.
  void SetRange(float min, float max);
  float GetRangeMin();
  float GetRangeMax();
  
  // Description:
  // Turn on display of the entry box widget that lets the user entry
  // an exact value.
  void DisplayEntry();

  // Description:
  // Set whether the entry is displayed to the side of the scale or on
  // top.  Default is 1 for on top.  Set to 0 for side.
  void SetDisplayEntryAndLabelOnTop(int value);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Check if the widget was modified.
  void CheckModifiedCallback();
  
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVScale* ClonePrototype(vtkPVSource* pvSource,
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
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // Called when menu item (above) is selected.  Neede for tracing.
  // Would not be necessary if menus traced invocations.
  void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai);
  
  // Description:
  // Get/Set whether to round floating point values to integers.
  vtkSetMacro(Round, int);
  vtkGetMacro(Round, int);
  vtkBooleanMacro(Round, int);
  
  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
protected:
  vtkPVScale();
  ~vtkPVScale();
  
  int RoundValue(float val);
  
  vtkKWLabel *LabelWidget;
  vtkKWScale *Scale;

  vtkPVScale(const vtkPVScale&); // Not implemented
  void operator=(const vtkPVScale&); // Not implemented

  vtkSetStringMacro(EntryLabel);
  vtkGetStringMacro(EntryLabel);
  char* EntryLabel;

  vtkSetStringMacro(RangeSourceVariable);
  vtkGetStringMacro(RangeSourceVariable);
  char* RangeSourceVariable;
  
  int Round;

  int AcceptedValueInitialized;

  vtkPVScalarListWidgetProperty *Property;
  
//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

};

#endif
