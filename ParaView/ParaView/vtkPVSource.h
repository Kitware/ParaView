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

class vtkKWEntry;
class vtkKWFrame;
class vtkKWLabeledEntry;
class vtkKWLabeledFrame;
class vtkKWLabeledLabel;
class vtkKWNotebook;
class vtkKWPushButton;
class vtkKWView;
class vtkKWWidget;
class vtkPVApplication;
class vtkPVData;
class vtkPVInputMenu;
class vtkPVLabel;
class vtkPVRenderView;
class vtkPVSourceCollection;
class vtkPVWidget;
class vtkPVWidgetCollection;
class vtkPVWindow;
class vtkSource;
class vtkStringList;

//BTX
template <class DType> 
class vtkVector;
//ETX

class VTK_EXPORT vtkPVSource : public vtkKWObject
{
public:
  static vtkPVSource* New();
  vtkTypeRevisionMacro(vtkPVSource,vtkKWObject);
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
  virtual void Pack();

  // Description: 
  // This flag turns the visibility of the prop on and off.
  // These methods transmit the state change to all of the satellite
  // processes.
  void SetVisibility(int v);
  int GetVisibility();

  // Description:
  // Although not all sources will need or use this input, I want to 
  // avoid duplicating VTK's source class structure.
  virtual void SetPVInput(vtkPVData *input);
  virtual vtkPVData* GetPVInput() {return this->GetNthPVInput(0);}
  vtkGetMacro(NumberOfPVInputs, int);
  vtkPVData *GetNthPVInput(int idx);

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
  vtkGetMacro(NumberOfPVOutputs, int);
  vtkPVData *GetNthPVOutput(int idx);
 
  // Description:
  // This name is used in the data list to identify the composite.
  virtual void SetName(const char *name);
  char* GetName();
    
  // Description:
  // The (short) description that can be used to give a more descriptive
  // name to the object. Note that if the description is empty when
  // GetDescription() is called, the Ivar is automatically initialized to
  // the name of the composite (GetName()).
  virtual void SetDescriptionNoTrace(const char *description);
  virtual void SetDescription(const char *description);
  virtual char* GetDescription();
  vtkGetObjectMacro(DescriptionFrame, vtkKWWidget);

  // Description:
  // Called when the description entry is changed.
  virtual void DescriptionEntryCallback();
    
  // Description:
  // This just returns the application typecast correctly.
  vtkPVApplication* GetPVApplication();  

  // Description:
  // Called when the accept button is pressed.
  void AcceptCallback();
  virtual void PreAcceptCallback();

  // Description:
  // Determine if this source can be deleted. This depends
  // on two conditions:
  // 1. Does the source have any consumers,
  // 2. Is it permanent (for example, a glyph source is permanent)
  virtual int IsDeletable();

  // Description:
  // Internal method; called by AcceptCallbackInternal.
  // Hide flag is used for hidding creation of 
  // the glyph sources from the user.
  virtual void Accept() { this->Accept(0); }
  virtual void Accept(int hideFlag) { this->Accept(hideFlag, 1); }
  virtual void Accept(int hideFlag, int hideSource);
  
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
  // The SetVTKSource method registers a ModifiedEvent observer with
  // the vtkSource to call this method.  This is used to keep the GUI
  // up to date as the vtkSource changes.
  void VTKSourceModifiedMethod();
  
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
  // Save the renderer and render window to a file.
  // The "vtkFlag" argument is only set when regression testing.
  // It causes the actors to b e added to the ParaView renderer. 
  virtual void SaveInTclScript(ofstream *file, int interactiveFlag, int vtkFlag);

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
  vtkPVData **GetPVInputs() { return this->PVInputs; };
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
  void AddInputClassName(const char* classname);
  vtkIdType GetNumberOfInputClasses();
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
  // Such modules should be marked as such (IsPermanent = 1).
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

  // Description:
  // If this is on, no Information page (from vtkPVData) is displayed
  // for this source. Used by sources like Glyphs.
  vtkSetMacro(HideInformationPage, int);
  vtkGetMacro(HideInformationPage, int);
  vtkBooleanMacro(HideInformationPage, int);

  // Description:
  // Raise the current source page.
  void RaiseSourcePage();

  // Description:
  // Set or get the module name. This name is used to store the
  // prototype in the sources/filters/readers maps. It is passed
  // to CreatePVSource when creating a new instance.
  vtkGetStringMacro(ModuleName);

  // Description:
  // Set or get the label to be used in the Source/Filter menus.
  vtkSetStringMacro(MenuName);
  vtkGetStringMacro(MenuName);

  // Description:
  // A short help string describing the module.
  vtkSetStringMacro(ShortHelp);
  vtkGetStringMacro(ShortHelp);

  // Description:
  // A longer help string describing the module.
  vtkSetStringMacro(LongHelp);
  vtkGetStringMacro(LongHelp);

  // Description:
  // This method returns the input source as a Tcl string.
  vtkPVSource* GetInputPVSource();

  // Description:
  // Check whether the source has been initialized 
  // (Accept has been called at least one)
  vtkGetMacro(Initialized, int);

  // Description:
  // If ToolbarModule is true, the instances of this module
  // can be created by using a button on the toolbar. This
  // variable is used mainly to prevent the addition of such
  // modules to Advanced->(Filter/Source) menus.
  vtkSetMacro(ToolbarModule, int);
  vtkGetMacro(ToolbarModule, int);
  vtkBooleanMacro(ToolbarModule, int);

  // Description:
  // When a module is first created, the user should not be able
  // to use certain menus, buttons etc. before accepting the first
  // time or deleting the module. GrabFocus() is used to tell the
  // module to disable certain things in the window.
  void GrabFocus();
  void UnGrabFocus();

protected:
  vtkPVSource();
  ~vtkPVSource();

  virtual void SerializeRevision(ostream& os, vtkIndent indent);
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is, const char token[1024]);

  // Description:
  // Create a menu to select the input.
  virtual vtkPVInputMenu *AddInputMenu(char* label, char* inputName, 
                                       char* inputType, char* help, 
                                       vtkPVSourceCollection* sources);

  vtkPVRenderView* GetPVRenderView();

  vtkSetStringMacro(ModuleName);
  
  // This flag gets set after the user hits accept for the first time.
  int Initialized;

  // If this is on, no Display page (from vtkPVData) is displayed
  // for this source. Used by sources like Glyphs
  int HideDisplayPage;

  // If this is on, no Paramters page  is displayed
  // for this source.
  int HideParametersPage;

  // If this is on, no Information page (from vtkPVData) is displayed
  // for this source. Used by sources like Glyphs
  int HideInformationPage;

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
 
  void RemoveAllPVInputs();
  
  void SetNthPVOutput(int idx, vtkPVData *output);
  void SetNthPVInput(int idx, vtkPVData *input);
  
  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();
  
  vtkKWNotebook *Notebook;

  // The name is just for display.
  char      *Name;
  char      *MenuName;
  char      *Description;
  char      *ShortHelp;
  char      *LongHelp;

  // This is the module name.
  char      *ModuleName;

  vtkKWWidget* Parameters;
  void UpdateProperties();

  vtkKWWidget *MainParameterFrame;
  vtkKWWidget *ButtonFrame;
  vtkKWFrame *ParameterFrame;
  
  vtkPVWidgetCollection *Widgets;

  vtkKWPushButton *AcceptButton;
  vtkKWPushButton *ResetButton;
  vtkKWPushButton *DeleteButton;

  vtkKWWidget *DescriptionFrame;
  vtkKWLabeledLabel *NameLabel;
  vtkKWLabeledLabel *TypeLabel;
  vtkKWLabeledEntry *DescriptionEntry;

  void UpdateDescriptionFrame();
    
  char *OutputClassName;
  char *SourceClassName;

  // Whether the source should make its input invisible.
  int ReplaceInput;

  int VisitedFlag;


  // Number of instances cloned from this prototype
  int PrototypeInstanceCount;

  int IsPermanent;
  
  // Flag for whether we are currently in AcceptCallback.
  int AcceptCallbackFlag;

  // Flag to tell whether the source is grabbed or not.
  int SourceGrabbed;

  virtual int ClonePrototypeInternal(int makeCurrent, vtkPVSource*& clone);

  int ToolbarModule;

//BTX
  vtkVector<const char*>* InputClassNames;
//ETX

private:
  vtkPVSource(const vtkPVSource&); // Not implemented
  void operator=(const vtkPVSource&); // Not implemented
};

#endif
