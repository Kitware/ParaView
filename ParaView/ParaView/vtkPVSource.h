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
// Note when there are multiple outputs, a dummy pvsource has to
// be attached to each of those. This way, the user can add modules
// after each output.


#ifndef __vtkPVSource_h
#define __vtkPVSource_h

#include "vtkKWObject.h"

class vtkPVSourceCollection;
class vtkPVWidgetCollection;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWWidget;
class vtkKWView;
class vtkKWNotebook;
class vtkKWLabeledFrame;
class vtkKWPushButton;
class vtkPVApplication;
class vtkPVData;
class vtkPVLabel;
class vtkPVRenderView;
class vtkPVRenderView;
class vtkKWLabeledFrame;
class vtkKWFrame;
class vtkKWLabel;
class vtkPVWidget;
class vtkPVWindow;
class vtkSource;
class vtkStringList;
class vtkPVInputMenu;

class VTK_EXPORT vtkPVSource : public vtkKWObject
{
public:
  static vtkPVSource* New();
  vtkTypeMacro(vtkPVSource,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the Window for this class.  This is used for creating input menus.
  // The PVSource list is in the window.
  vtkPVWindow *GetPVWindow();
  
  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties();
  
  // Description:
  // Methods to indicate when this source is selected in the window..
  // The second version of Deselect allows to user to specify whether
  // the main frame of the source parameters should be unpacked or
  // not. It was necessary to add this option to work around some
  // Tk packing problems.
  virtual void Select();
  virtual void Deselect() { this->Deselect(1); }
  virtual void Deselect(int doPackForget);

  // Description: 
  // This flag turns the visibility of the prop on and off.
  // These methods transmit the state change to all of the satellite
  // processes.
  void SetVisibility(int v);

  // Description:
  // Although not all sources will need or use this input, I want to 
  // avoid duplicating VTK's source class structure.
  virtual void SetPVInput(vtkPVData *input);
  virtual vtkPVData* GetPVInput() {return this->GetNthPVInput(0);}

  // Description:
  // Set/get the first output of this source. Most source are setup
  // with only one output.
  void SetPVOutput(vtkPVData *pvd);
  vtkPVData* GetPVOutput() { return this->GetNthPVOutput(0);}

  // Description:
  // Set/get the nth output of this source. These are not commonly
  // used since most of the source have only one output.
  void SetPVOutput(int idx, vtkPVData *pvd) {this->SetNthPVOutput(idx,pvd);}
  vtkPVData *GetPVOutput(int idx) { return this->GetNthPVOutput(idx); }

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
  // Determine if this source can be deleted. This depends
  // on two conditions:
  // 1. Does the source have any consumers,
  // 2. Is it permanent (for example, a glyph source is permanent)
  virtual int IsDeletable();

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
  // This method resets the UI values (Widgets added with the following
  // methods).  It uses the GetCommand supplied to the interface.
  virtual void UpdateParameterWidgets();
  
  // Description:
  // This method gets called to set the VTK source parameters
  // from the widget values.
  virtual void UpdateVTKSourceParameters();

  // Description:
  // Set the vtk source that will be a part of the pipeline.
  // The pointer to this class is used as little as possible.
  // (VTKSourceTclName is used instead.)
  void SetVTKSource(vtkSource *source, const char *tclName);
  vtkGetObjectMacro(VTKSource, vtkSource);
  vtkGetStringMacro(VTKSourceTclName);

  vtkGetObjectMacro(DeleteButton, vtkKWPushButton);
  vtkGetObjectMacro(AcceptButton, vtkKWPushButton);
  
  vtkGetObjectMacro(Widgets, vtkPVWidgetCollection);
  
  vtkGetObjectMacro(ParameterFrame, vtkKWFrame);
  vtkGetObjectMacro(MainParameterFrame, vtkKWWidget);
  
  // Description:
  // Used to save the source into a file.
  void SaveInTclScript(ofstream *file);

  // Description:
  // Make the Accept button turn red/white when one of the parameters 
  // has changed.
  void SetAcceptButtonColorToRed();
  void SetAcceptButtonColorToWhite();
  
  // Description:
  // Needed to clean up properly.
  vtkSetStringMacro(ExtentTranslatorTclName);
  vtkGetStringMacro(ExtentTranslatorTclName);

  // Description:
  // This flag determines whether a source will make its input invisible or
  // not.  By default, this flag is on.
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
  void SetView(vtkKWView* view);
  vtkGetObjectMacro(View, vtkKWView);
  vtkKWView *View;

  // Description:
  // This allows you to set the propertiesParent to any widget you like.  
  // If you do not specify a parent, then the views->PropertyParent is used.  
  // If the composite does not have a view, then a top level window is created.
  void SetParametersParent(vtkKWWidget *parent);
  vtkGetObjectMacro(ParametersParent, vtkKWWidget);
  vtkKWWidget *ParametersParent;

  // Desription:
  // This is just a flag that is used to mark that the source has been saved
  // into the tcl script (visited) during the recursive saving process.
  vtkSetMacro(VisitedFlag,int);
  vtkGetMacro(VisitedFlag,int);

  // Description:
  // This adds the PVWidget and sets up the callbacks to initialize its trace.
  void AddPVWidget(vtkPVWidget *pvw);

  // Description: Creates and returns (by reference) a copy of this
  // source. It will create a new instance of the same type as the current
  // object using NewInstance() and then call ClonePrototype() on all
  // widgets and add these clones to it's widget list. The return
  // value is VTK_OK is the cloning was successful.
  int ClonePrototype(int makeCurrent, vtkPVSource*& clone);

  // Description:
  // This method can be used by subclasses of PVSource to do
  // special initialization (for example creating widgets). 
  // It is called by PVWindow before adding a prototype to the main list.
  virtual void InitializePrototype() {}

  // Description:
  // For now we are supporting only one input and output.
  vtkSetStringMacro(InputClassName);
  vtkGetStringMacro(InputClassName);
  vtkSetStringMacro(OutputClassName);
  vtkGetStringMacro(OutputClassName);
  vtkSetStringMacro(SourceClassName);
  vtkGetStringMacro(SourceClassName);

  // Description:
  // This method is called by the window to determine if this filter should be
  // added to the filter menu.  Right now, only the class name of the input
  // is checked.  In the future, attributes could be checked as well.
  virtual int GetIsValidInput(vtkPVData *input);

  // Description:
  // Certain modules are not deletable (for example, glyph sources).
  // Such modules should be marked as such (IsPermanent = 1)x.
  vtkSetMacro(IsPermanent, int);
  vtkGetMacro(IsPermanent, int);
  vtkBooleanMacro(IsPermanent, int);

  // Description:
  // Returns the number of consumers of either:
  // 1. the first output if there is only one output,
  // 2. the outputs of the consumers of
  //    each output if there are more than one outputs.
  // Method (2) is used for multiple outputs because,
  // in this situation, there is always a dummy pvsource
  // attached to each of the outputs.
  int GetNumberOfPVConsumers();

  // Description:
  // If this is on, no Display page (from vtkPVData) is display
  // for this source. Used by sources like Glyphs.
  vtkSetMacro(HideDisplayPage, int);
  vtkGetMacro(HideDisplayPage, int);
  vtkBooleanMacro(HideDisplayPage, int);

  // Description:
  // If this is on, no Paramters page  is displayed for this source.
  vtkSetMacro(HideParametersPage, int);
  vtkGetMacro(HideParametersPage, int);
  vtkBooleanMacro(HideParametersPage, int);

protected:
  vtkPVSource();
  ~vtkPVSource();

  // Description:
  // Create a menu to select the input.
  virtual vtkPVInputMenu *AddInputMenu(char* label, char* inputName, 
				       char* inputType, char* help, 
				       vtkPVSourceCollection* sources);

  vtkPVRenderView* GetPVRenderView();
  
  // This flag gets set after the user hits accept for the first time.
  int Initialized;

  // If this is on, no Display page (from vtkPVData) is displayed
  // for this source. Used by sources like Glyphs
  int HideDisplayPage;

  // If this is on, no Paramters page  is displayed
  // for this source.
  int HideParametersPage;

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
 
  vtkPVData *GetNthPVInput(int idx);
  void RemoveAllPVInputs();
  
  vtkPVData *GetNthPVOutput(int idx);
  void SetNthPVOutput(int idx, vtkPVData *output);
  void SetNthPVInput(int idx, vtkPVData *input);

  vtkKWNotebook *Notebook;

  // The name is just for display.
  char      *Name;

  vtkKWWidget* Parameters;
  void UpdateProperties();

  vtkKWWidget *MainParameterFrame;
  vtkKWWidget *ButtonFrame;
  vtkKWFrame *ParameterFrame;
  
  vtkPVWidgetCollection *Widgets;


  vtkKWPushButton *AcceptButton;
  vtkKWPushButton *ResetButton;
  vtkKWPushButton *DeleteButton;
  vtkKWLabel *DisplayNameLabel;
    
  char *InputClassName;
  char *OutputClassName;
  char *SourceClassName;

  // Whether the source should make its input invisible.
  int ReplaceInput;

  int VisitedFlag;

  vtkPVSource(const vtkPVSource&); // Not implemented
  void operator=(const vtkPVSource&); // Not implemented

  // Number of instances cloned from this prototype
  int PrototypeInstanceCount;

  int IsPermanent;

  virtual int ClonePrototypeInternal(int makeCurrent, vtkPVSource*& clone);
};

#endif
