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

#include "vtkKWObject.h"
#include "vtkKWView.h"

class vtkCollection;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWPushButton;
class vtkPVApplication;
class vtkPVArrayMenu;
class vtkPVArraySelection;
class vtkPVBoundsDisplay;
class vtkPVData;
class vtkPVFileEntry;
class vtkPVInputMenu;
class vtkPVLabel;
class vtkPVLabeledToggle;
class vtkPVRenderView;
class vtkPVScalarRangeLabel;
class vtkPVScale;
class vtkPVSelectionList;
class vtkPVSourceInterface;
class vtkPVRenderView;
class vtkKWLabeledFrame;
class vtkKWScrollableFrame;
class vtkKWLabel;
class vtkPVStringEntry;
class vtkPVVectorEntry;
class vtkPVWidget;
class vtkPVWindow;
class vtkSource;
class vtkStringList;

class VTK_EXPORT vtkPVSource : public vtkKWObject
{
public:
  static vtkPVSource* New();
  vtkTypeMacro(vtkPVSource,vtkKWObject);
  
  // Description:
  // Get the Window for this class.  This is used for creating input menus.
  // The PVSource list is in the window.
  vtkPVWindow *GetPVWindow();
  
  // Description:
  // A way to get the output in the superclass.
  vtkPVData *GetPVOutput(int idx) { return this->GetNthPVOutput(idx); }
    
  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties();
  
  // Description:
  // Methods to indicate when this source is selected in the window..
  virtual void Select();
  virtual void Deselect();

  // Description:
  // This flag turns the visibility of the prop on and off.  These methods transmit
  // the state change to all of the satellite processes.
  void SetVisibility(int v);

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
  // Called when the accept button is pressed.
  virtual void AcceptCallback();
  virtual void PreAcceptCallback();

  // Description:
  // Internal method; called by AcceptCallback.
  // Hide flag is used for hidding creation of 
  // the glyph sources from the user.
  void Accept(int hideFlag);
  
  // Description:
  // Called when the reset button is pressed.
  void ResetCallback();

  // Description:
  // Called when the delete button is pressed.
  virtual void DeleteCallback();

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
  // This widget is for readers that selectively read arrays.
  vtkPVArraySelection *AddArraySelection(const char *attributeName, 
                                         const char *help);

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
                                    char* help, int dataType);
  
  // Description:
  // Add label to vtkSource
  vtkPVLabel *AddLabel(char* label, char* help);

  // Description:
  // Create an entry for items with multiple elements.
  // The primary label is put to the left.  The element labels
  // (l1,l2,l3, ...) are put in from of the individual entry boxes.
  // The methods are called on the object (VTKSource if o=NULL).
  vtkPVVectorEntry* AddVector2Entry(char *label, char *l1, char *l2,
                                    char *varName, char* help, int dataType);
  vtkPVVectorEntry* AddVector3Entry(char *label, char *l1, char *l2, char *l3,
                                    char *varName, char* help, int dataType);
  vtkPVVectorEntry* AddVector4Entry(char *label, char *l1, char *l2, char *l3,
                                    char *l4, char *varName, char *help, int dataType);
  vtkPVVectorEntry* AddVector6Entry(char *label, char *l1, char *l2, char *l3, 
                                    char *l4, char *l5, char *l6,
                                    char *varName, char* help, int dataType);
  
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
  
  vtkGetObjectMacro(ParameterFrame, vtkKWScrollableFrame);
  vtkGetObjectMacro(MainParameterFrame, vtkKWWidget);
  
  // Description:
  // Used to save the source into a file.
  void SaveInTclScript(ofstream *file);

  // Description:
  // This will be the new way the source gets specified.  It will use the
  // interface directly.
  void SetInterface(vtkPVSourceInterface *pvsi);
  vtkPVSourceInterface *GetInterface() {return this->Interface;}

  // Description:
  // Make the Accept button turn red/white when one of the parameters 
  // has changed.
  void SetAcceptButtonColorToRed();
  void SetAcceptButtonColorToWhite();
  
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

  // Description:
  // The notebook that is displayed when the source is selected.
  vtkGetObjectMacro(Notebook, vtkKWNotebook);

  // I shall want to get rid of this.
  vtkSetObjectMacro(View, vtkKWView);
  vtkGetObjectMacro(View, vtkKWView);
  vtkKWView *View;

  // Description:
  // This allows you to set the propertiesParent to any widget you like.  
  // If you do not specify a parent, then the views->PropertyParent is used.  
  // If the composite does not have a view, then a top level window is created.
  void SetPropertiesParent(vtkKWWidget *parent);
  vtkGetObjectMacro(PropertiesParent, vtkKWWidget);
  vtkKWWidget *PropertiesParent;

  // Desription:
  // This is just a flag that is used to mark that the source has been saved
  // into the tcl script (visited) during the recursive saving process.
  vtkSetMacro(VisitedFlag,int);
  vtkGetMacro(VisitedFlag,int);

  // Description:
  // This adds the PVWidget and sets up the callbacks to initialize its trace.
  void AddPVWidget(vtkPVWidget *pvw);

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
    
  
  vtkKWNotebook *Notebook;

  // The name is just for display.
  char      *Name;

  vtkKWWidget       *Properties;
  void              UpdateProperties();

  vtkKWWidget *MainParameterFrame;
  vtkKWWidget *ButtonFrame;
  vtkKWScrollableFrame *ParameterFrame;
  
  vtkKWWidgetCollection *Widgets;


  vtkKWPushButton *AcceptButton;
  vtkKWPushButton *ResetButton;
  vtkKWPushButton *DeleteButton;
  vtkKWLabel *DisplayNameLabel;
    
  vtkPVSelectionList *LastSelectionList;

  vtkPVSourceInterface *Interface;
  
  // Whether the source should make its input invisible.
  int ReplaceInput;

  int InstanceCount;
  int VisitedFlag;
};

#endif
