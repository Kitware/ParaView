/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVVectorEntry.h
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
// .NAME vtkPVVectorEntry - ParaView entry widget.
// .SECTION Description
// This widget allows user to enter text. It has a label and can have
// multiple fields. The maximum number of fields is 6. Once the widget
// is created the number of fields cannot be changed. When the user
// modifies the entry, the modified callback is called.

#ifndef __vtkPVVectorEntry_h
#define __vtkPVVectorEntry_h

#include "vtkPVObjectWidget.h"

class vtkStringList;
class vtkKWEntry;
class vtkKWApplication;
class vtkKWLabel;
class vtkKWWidgetCollection;

//BTX
template<class KeyType,class DataType> class vtkArrayMap;
//ETX

class VTK_EXPORT vtkPVVectorEntry : public vtkPVObjectWidget
{
public:
  static vtkPVVectorEntry* New();
  vtkTypeMacro(vtkPVVectorEntry, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Create(vtkKWApplication *pvApp);
  
  // Description:
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Adds a trace entry.  Side effect is to turn modified flag off.
  virtual void Accept();
  
  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void Reset();

  // Description:
  // I will eventually remove access to internal widgets once I figure
  // out how to get the vectors value in Tcl with any number of componenets.
  vtkGetObjectMacro(LabelWidget, vtkKWLabel);
  vtkGetObjectMacro(SubLabels, vtkKWWidgetCollection);
  vtkGetObjectMacro(Entries, vtkKWWidgetCollection);
  vtkKWLabel* GetSubLabel(int idx);
  vtkKWEntry* GetEntry(int idx);

  // Description:
  // Methods to set this widgets value from a script.
  void SetValue(char* v);
  void SetValue(char* v1, char* v2);
  void SetValue(char* v1, char* v2, char* v3);
  void SetValue(char* v1, char* v2, char* v3, char* v4);
  void SetValue(char* v1, char* v2, char* v3, char* v4, char* v5);
  void SetValue(char* v1, char* v2, char* v3, char* v4, char* v5, char* v6);
  void SetValue(char** vals, int num);
  void SetValue(float* vals, int num);

  // Description:
  // Access values in the vector. Argument num is the size of the
  // values array. It has to be smaller than number of items.
  void GetValue(float* values, int num);
  float *GetValue6();

  // Description:
  // Check if the entry was modified and call modified event.
  void CheckModifiedCallback(const char*);
  
  // Description:
  // I need a solution:  I want to run ParaView with a low resolution
  // data set, but create a batch simulation with high resolution data.
  //  When this widget is saved in a VTK script, this value is used.
  vtkSetStringMacro(ScriptValue);
  vtkGetStringMacro(ScriptValue);

  // Description:
  // An interface for saving a widget into a script.
  virtual void SaveInTclScript(ofstream *file);

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, vtkPVAnimationInterface *ai);

  // Description:
  // This is the data type the vtk object is expecting.
  vtkSetMacro(DataType, int); 
  vtkGetMacro(DataType, int); 
  
  // Description:
  // The label.
  void SetLabel(const char* label);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Sets the length of the vector
  vtkSetMacro(VectorLength, int);
  vtkGetMacro(VectorLength, int);

  // Description:
  // Sets one of the sub-labels. This has to be done
  // before create.
  void SetSubLabel(int i, const char* sublabl);

  // Description:
  // Set the entry value.
  void SetEntryValue(int index, const char* value);

  // Description:
  // Set or get whether the entry is read only or not.
  vtkSetClampMacro(ReadOnly, int, 0, 1);
  vtkBooleanMacro(ReadOnly, int);
  vtkGetMacro(ReadOnly, int);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVVectorEntry* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

protected:
  vtkPVVectorEntry();
  ~vtkPVVectorEntry();
  
  vtkKWLabel *LabelWidget;
  vtkKWWidgetCollection *SubLabels;
  vtkKWWidgetCollection *Entries;

  vtkSetStringMacro(EntryLabel);
  vtkGetStringMacro(EntryLabel);
  char* EntryLabel;

  int DataType;
  int VectorLength;

  // Description
  // Set this to 1 to be read only
  int ReadOnly;

  char *ScriptValue;

  // Description:
  // Get the stored entry values.
  char *EntryValues[6];

  vtkPVVectorEntry(const vtkPVVectorEntry&); // Not implemented
  void operator=(const vtkPVVectorEntry&); // Not implemented

  vtkStringList* SubLabelTxts;

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
