/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVFileEntry.h
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
// .NAME vtkPVFileEntry -
// .SECTION Description

#ifndef __vtkPVFileEntry_h
#define __vtkPVFileEntry_h

#include "vtkPVObjectWidget.h"

class vtkKWLabel;
class vtkKWPushButton;
class vtkKWEntry;
class vtkPVSource;
class vtkKWScale;
class vtkKWFrame;
class vtkPVFileEntryProperty;
class vtkKWListSelectOrder;

//BTX
template<class KeyType,class DataType> class vtkArrayMap;
//ETX

class VTK_EXPORT vtkPVFileEntry : public vtkPVObjectWidget
{
public:
  static vtkPVFileEntry* New();
  vtkTypeRevisionMacro(vtkPVFileEntry, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The label can be set before or after create is called.
  void SetLabel(const char* label);
  const char* GetLabel();

  virtual void Create(vtkKWApplication *pvApp);
  
  // Description:
  // This method allows scripts to modify the widgets value.
  virtual void SetValue(const char* fileName);
  const char* GetValue();

  // Description:
  // Called when the browse button is pressed.
  void BrowseCallback();

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // The extension used in the file dialog
  vtkSetStringMacro(Extension);
  vtkGetStringMacro(Extension);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVFileEntry* ClonePrototype(vtkPVSource* pvSource,
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
  // This callback is called when entry changes.
  void EntryChangedCallback();

  // Description:
  // This callback is called when timestep changes.
  void TimestepChangedCallback();

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                         vtkPVAnimationInterfaceEntry *ai);

  // Description:
  // This method gets called when the user selects this widget to animate.
  // It sets up the script and animation parameters.
  void AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai);


  // Description:
  // Set the current time step.
  void SetTimeStep(int ts);

  // Description:
  // Used internally. Method to save widget parameters into vtk tcl script.
  void SaveInBatchScriptForPart(ofstream* file, vtkClientServerID);

  // Description:
  // Get the range of files.
  vtkGetVector2Macro(Range, int);

  // Description:
  // Set/get the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* GetProperty();
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
protected:
  vtkPVFileEntry();
  ~vtkPVFileEntry();

  vtkKWLabel *LabelWidget;
  vtkKWPushButton *BrowseButton;
  vtkKWEntry *Entry;

  char* Extension;
  int InSetValue;

  // Timestep scale
  vtkKWFrame *TimestepFrame;
  vtkKWScale *Timestep;
  int TimeStep;

  vtkKWListSelectOrder* FileListSelect;

  vtkSetStringMacro(Format);
  vtkSetStringMacro(Prefix);
  vtkSetStringMacro(Path);
  vtkSetStringMacro(Ext);
  vtkSetVector2Macro(Range, int);

  char* Prefix;
  char* Format;
  char* Path;
  char* Ext;
  int FileNameLength;
  int Range[2];

  vtkPVFileEntryProperty *Property;

  //BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
    vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  //ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
    vtkPVXMLPackageParser* parser);
  
private:
  vtkPVFileEntry(const vtkPVFileEntry&); // Not implemented
  void operator=(const vtkPVFileEntry&); // Not implemented
};

#endif
