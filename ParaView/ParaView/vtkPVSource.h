/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSource.h
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
// .NAME vtkPVSource - A super class for filter objects.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.
// This class should probably be merged with vtkPVComposite.


#ifndef __vtkPVSource_h
#define __vtkPVSource_h

#include "vtkKWComposite.h"

class vtkKWPushButton;
class vtkPVArraySelection;
class vtkPVApplication;
class vtkPVInputMenu;
class vtkPVArrayMenu;
class vtkSource;
class vtkPVData;
class vtkPVWindow;
class vtkStringList;
class vtkPVLabeledToggle;
class vtkPVFileEntry;
class vtkPVStringEntry;
class vtkPVVectorEntry;
class vtkPVScale;
class vtkKWEntry;
class vtkPVSelectionList;
class vtkPVSourceInterface;
class vtkPVRenderView;
class vtkKWLabeledFrame;
class vtkKWLabel;
class vtkPVWidget;
class vtkPVBoundsDisplay;
class vtkPVScalarRangeLabel;

class VTK_EXPORT vtkPVSource : public vtkKWComposite
{
public:
  static vtkPVSource* New();
  vtkTypeMacro(vtkPVSource,vtkKWComposite);
  
  // Description:
  // Get the Window for this class.
  vtkPVWindow *GetPVWindow();
  
  // Description:
  // A way to get the output in the superclass.
  vtkPVData *GetPVOutput(int idx) { return this->GetNthPVOutput(idx); }
    
  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties();
  
  // Description:
  // Methods to indicate when this composite is the selected composite.
  // These methods are used by subclasses to modify the menu bar
  // for example. When a volume composite is selected it might 
  // add an option to the menu bar to view the 2D slices.
  virtual void Select(vtkKWView *);
  virtual void Deselect(vtkKWView *);

  // Description:
  // This flag turns the visibility of the prop on and off.  These methods transmit
  // the state change to all of the satellite processes.
  void SetVisibility(int v);
  int GetVisibility();
  vtkBooleanMacro(Visibility, int);

  // Description:
  // Although not all sources will need or use this input, I want to 
  // avoid duplicating VTK's source class structure.
  virtual void SetPVInput(vtkPVData *input);
  virtual vtkPVData* GetPVInput() {return this->GetNthPVInput(0);}

  // Description:
  // Sources are currently setup to have exactly one output.
  void SetPVOutput(vtkPVData *pvd);
  vtkPVData* GetPVOutput() { return this->GetNthPVOutput(0);}

  // Description:
  // This name is used in the data list to identify the composite.
  virtual void SetName(const char *name);
  char* GetName();
    
  // Description:
  // This just returns the application typecast correctly.
  vtkPVApplication* GetPVApplication();  

  // Description:
  // Sources have no props.
  vtkProp *GetProp() {return NULL;}  

  // Description:
  // Called when the accept button is pressed.
  virtual void AcceptCallback();
  virtual void PreAcceptCallback();

  // Description:
  // Internal method; called by AcceptCallback
  void Accept();
  
  // Description:
  // Called when the reset button is pressed.
  void ResetCallback();

  // Description:
  // Called when the delete button is pressed.
  virtual void DeleteCallback();

  // Description:
  // A call back method from the Navigation window.
  // This sets the input source as the selected composite.
  void SelectSource(vtkPVSource *source);

  // Description:
  // This method resets the UI values (Widgets added with the following methods).
  // It uses the GetCommand supplied to the interface.
  virtual void UpdateParameterWidgets();
  
  // Description:
  // This method gets called to set the VTK source parameters
  // from the widget values.
  virtual void UpdateVTKSourceParameters();

  //---------------------------------------------------------------------
  // This is a poor way to create widgets.  Another method should be created.
  // Well, these do not seem so bad as conveniance methods. 
  
  // Description:
  // Create a menu to select the input.
  vtkPVInputMenu *AddInputMenu(char* label, char* inputName, char* inputType,
                               char* help, vtkCollection* sources);

  // Description:
  // Adds a widget that displays the bounds of the input.
  vtkPVBoundsDisplay* AddBoundsDisplay(vtkPVInputMenu *inputMenu);

  // Description:
  // Create a menu to select the active scalars of the input..
  vtkPVArrayMenu *AddArrayMenu(const char* label, int attributeType, 
                               int numComponents, const char* help);

  // Description:
  // Adds a widget that displays the scalar range of the input array.
  vtkPVScalarRangeLabel* AddScalarRangeLabel(vtkPVArrayMenu *arrayMenu);

  // Description:
  // Create an entry for a filename.
  vtkPVFileEntry *AddFileEntry(char* label, char* varName,
                               char* ext, char* help);

  // Description:
  // Formats the command with brackets so that sopaces are preserved.  
  // Label is put to left of entry.
  // The methods are called on the object (VTKSource if o=NULL).
  vtkPVStringEntry *AddStringEntry(char* label, char* varName,
                                   char* help);
 
  // Description:
  // Create an entry for a single value.  Label is put to left of entry.
  // The methods are called on the object (VTKSource if o=NULL).
  vtkPVVectorEntry *AddLabeledEntry(char* label, char* varName,
                                    char* help);
  
  // Description:
  // Create an entry for items with multiple elements.
  // The primary label is put to the left.  The element labels
  // (l1,l2,l3, ...) are put in from of the individual entry boxes.
  // The methods are called on the object (VTKSource if o=NULL).
  vtkPVVectorEntry* AddVector2Entry(char *label, char *l1, char *l2,
                                    char *varName, char* help);
  vtkPVVectorEntry* AddVector3Entry(char *label, char *l1, char *l2, char *l3,
                                    char *varName, char* help);
  vtkPVVectorEntry* AddVector4Entry(char *label, char *l1, char *l2, char *l3,
                                    char *l4, char *varName, char *help);
  vtkPVVectorEntry* AddVector6Entry(char *label, char *l1, char *l2, char *l3, 
                                    char *l4, char *l5, char *l6,
                                    char *varName, char* help);
  
  // Description:
  // Special widget controls (not entries).
  // The methods are called on the object (VTKSource if o=NULL).
  vtkPVLabeledToggle *AddLabeledToggle(char *label, char *varName, char* help);
  vtkPVScale *AddScale(char *label, char *varName, float min, float max, 
                       float resolution, char* help);
  
  // Description:
  // Creates a list for selecting a mode.
  // The methods are called on the object (VTKSource if o=NULL).
  vtkPVSelectionList *AddModeList(char *label, char *varName, char* help);
  void AddModeListItem(char *name, int value);

  // Description:
  // Set the vtk source that will be a part of the pipeline.
  // The pointer to this class is used as little as possible.
  // (VTKSourceTclName is used instead.)
  void SetVTKSource(vtkSource *source, const char *tclName);
  vtkGetObjectMacro(VTKSource, vtkSource);
  vtkGetStringMacro(VTKSourceTclName);

  vtkGetObjectMacro(DeleteButton, vtkKWPushButton);
  vtkGetObjectMacro(AcceptButton, vtkKWPushButton);
  
  vtkGetObjectMacro(Widgets, vtkKWWidgetCollection);
  
  vtkGetObjectMacro(ParameterFrame, vtkKWLabeledFrame);
  
  // Description:
  // Used to save the source into a file.
  void SaveInTclScript(ofstream *file);

  // Description:
  // This will be the new way the source gets specified.  It will use the
  // interface directly.
  void SetInterface(vtkPVSourceInterface *pvsi);
  vtkPVSourceInterface *GetInterface() {return this->Interface;}

  // Description:
  // Make the Accept button turn red when one of the parameters has changed.
  void ChangeAcceptButtonColor();
  
  // Description:
  // Needed to clean up properly.
  vtkSetStringMacro(ExtentTranslatorTclName);

  // Description:
  // This flag determines whether a source will make its input invisible or not.
  // By default, this flag is on.
  vtkSetMacro(ReplaceInput, int);
  vtkGetMacro(ReplaceInput, int);
  vtkBooleanMacro(ReplaceInput, int);

  // Description:
  // Access to PVWidgets indexed by their name.
  vtkPVWidget *GetPVWidget(const char *name);

  // Description:
  // These are used for drawing the navigation window.
  vtkGetMacro(NumberOfPVInputs, int);
  vtkPVData **GetPVInputs() { return this->PVInputs; };
  vtkGetMacro(NumberOfPVOutputs, int);
  vtkPVData **GetPVOutputs() { return this->PVOutputs; };

protected:
  vtkPVSource();
  ~vtkPVSource();
  vtkPVSource(const vtkPVSource&) {};
  void operator=(const vtkPVSource&) {};

  // What did this do ??? ... 
  char *DefaultScalarsName;
  vtkSetStringMacro(DefaultScalarsName);

  vtkPVRenderView* GetPVRenderView();
  
  // This flag gets set after the user hits accept for the first time.
  int Initialized;
  
  vtkSource *VTKSource;
  char *VTKSourceTclName;
  
  // To clean up properly.
  char *ExtentTranslatorTclName;

  // Called to allocate the output array.  Copies old outputs.
  void SetNumberOfPVOutputs(int num);
  vtkPVData **PVOutputs;
  int NumberOfPVOutputs;  
  
  // Called to allocate the input array.  Copies old inputs.
  void SetNumberOfPVInputs(int num);
  vtkPVData **PVInputs;
  int NumberOfPVInputs; 
 
  void SetNthPVInput(int idx, vtkPVData *input);
  vtkPVData *GetNthPVInput(int idx);
  void RemoveAllPVInputs();
  
  // Description:
  // Although it looks like there is support for multiple outputs,
  // there are several implementation details that assume only one output.
  vtkPVData *GetNthPVOutput(int idx);
  void SetNthPVOutput(int idx, vtkPVData *output);
 
 
  // We keep a reference to the input menu (if created) because any
  // array menu must be dependant on it.
  // This is a bit of a hack until we represent the dependancey in the XML.
  vtkPVInputMenu *InputMenu;
    
  
  // The name is just for display.
  char      *Name;

  vtkKWWidget       *Properties;
  void              UpdateProperties();

  vtkKWLabeledFrame *ParameterFrame;
  
  vtkKWWidgetCollection *Widgets;

  // Description:
  // This adds the PVWidget and sets up the callbacks to initialize its trace.
  void AddPVWidget(vtkPVWidget *pvw);

  vtkKWPushButton *AcceptButton;
  vtkKWPushButton *ResetButton;
  vtkKWPushButton *DeleteButton;
  vtkKWLabel *DisplayNameLabel;
    
  vtkPVSelectionList *LastSelectionList;

  vtkPVSourceInterface *Interface;
  
  // Whether the source should make its input invisible.
  int ReplaceInput;

  int InstanceCount;
};

#endif
