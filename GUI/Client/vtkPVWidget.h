/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWidget - Intelligent widget for building source interfaces.
// .SECTION Description
// vtkPVWidget is a superclass for widgets that can be used to build
// interfaces for vtkPVSources.  These widgets combine the UI of vtkKWWidgets,
// but can synchronize themselves with vtkObjects.  When the "Accept" method
// is called, this widgets sets the vtkObjects ivars based on the widgets
// value.  When the "Reset" method is called, the vtkObjects state is used to 
// set the widgets value.  When the widget has been modified, and the "Accept"
// method is called,  the widget will save the transition in the trace file.

#ifndef __vtkPVWidget_h
#define __vtkPVWidget_h

#include "vtkPVTracedWidget.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID
class vtkKWMenu;
class vtkPVSource;
class vtkPVApplication;
class vtkPVXMLElement;
class vtkPVXMLPackageParser;
class vtkPVWindow;
class vtkSMProperty;
class vtkPVWidgetObserver;

//BTX
template <class key, class data> 
class vtkArrayMap;
template <class value>
class vtkLinkedList;
//ETX

class VTK_EXPORT vtkPVWidget : public vtkPVTracedWidget
{
public:
  vtkTypeRevisionMacro(vtkPVWidget, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These methods are called when the Accept and Reset buttons are pressed.
  // The copy state from VTK/PV objects to the widget and back.
  // Most subclasses do not have to implement these methods.  They implement
  // AcceptInternal and ResetInternal instead.
  // Only methods that copy state from PV object need 
  // to override these methods.
  // Accept needs to add to the trace (call trace), but AcceptInternal does not.
  virtual void Accept()=0;
  virtual void PostAccept() {};
  virtual void Reset();

  // Description:
  // Called right after the widgets are created, this virtual method allows
  // widgets to set appropriate initial values, 3d widgets to place themselves
  // etc. 
  virtual void Initialize() = 0;

  // Description:
  // The methods get called when reset is called.  
  // It can also get called on its own.  If the widget has options 
  // or configuration values dependent on the VTK object, this method
  // set these configuation object using the VTK object.
  virtual void Update();

  // Description:
  // Overwritten by sub-classes. Any widget that contains a proxy
  // should call UpdateVTKObjects on it in this method.
  virtual void UpdateVTKObjects() {};

  // Description:
  // Widgets that depend on the value of this widget can 
  // set up a dependance here.  When ModifedCallback or Update is called 
  // on this widget, it will call Update on widgets in this list.  
  // I could have used event callbacks, but descided
  // it would be easier to just keep a collection of dependances.
  void AddDependent(vtkPVWidget *widget);

  // Description
  // Remove a dependent
  void RemoveDependent(vtkPVWidget *widget);

  // Description
  // Remove all the dependents
  void RemoveAllDependents();

  // Description:
  // This method is called when the source that contains this widget
  // is selected. This is empty implementation but subclasses can
  // overwrite it.
  virtual void Select() {}

  // Description:
  // This method is called when the source that contains this widget
  // is deselected. This is empty implementation but subclasses can
  // overwrite it.
  virtual void Deselect() {}

  // Description:
  // This commands is an optional action that will be called when
  // the widget is modified.  Really we should change "ModifiedFlag" 
  // to some other name, because is gets confused with vtkObject::Modified().
  void SetModifiedCommand(const char* cmdObject, const char* methodAndArgs);
  void SetAcceptedCommand(const char* cmdObject, const char* methodAndArgs);

  // Description:
  // This callback gets called when the user changes the widgets value,
  // or a script changes the widgets value.  Ideally, this method should 
  // be protected.
  virtual void ModifiedCallback();

  // Description:
  // This callback gets called when the widget is accepted.
  virtual void AcceptedCallback();

  // Description:
  // Access to the flag that indicates whether the widgets
  // has been modified and is out of sync with its VTK object.
  vtkGetMacro(ModifiedFlag,int);
  
  // Description:
  // Conveniance method that casts the application to a PV application.
  vtkPVApplication *GetPVApplication();

  // Description:
  // Save this widget to a file.
  virtual void SaveInBatchScript(ofstream *file);

  // Description:
  // This method calls "Trace" to save this widget into a state file.
  // This method is not virtual and sublclasses do not have to implement 
  // this method.  Subclasses define the interal "Trace" which works for
  // saving state and tracing.
  void SaveState(ofstream *file);

  // Description:
  // Create the widget. All sub-classes should use this
  // signature because widgets are created using vtkPVWidget
  // pointers after cloning.
  virtual void Create(vtkKWApplication* app) = 0;

  // Description:
  // Need the source to get the input.
  // I would like to get rid of this ivar.
  // It is not reference counted for fear of loops.
  void SetPVSource(vtkPVSource *pvs) { this->PVSource = pvs;}
  vtkPVSource *GetPVSource() { return this->PVSource;}

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVWidget* ClonePrototype(vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
    {
      return this->ClonePrototypeInternal(pvSource, map);
    }
//ETX

  // Description:
  // Called by vtkPVXMLPackageParser to configure the widget from XML
  // attributes.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);

  // Description:
  // Get the tcl object and the method that will be called when
  // accepting or reseting the source.
  vtkGetStringMacro(ModifiedCommandObjectTclName);
  vtkGetStringMacro(ModifiedCommandMethod);

  // Description:
  // Used by subclasses to save this widgets state into a PVScript.
  // This method does not initialize trace variable or check modified.
  virtual void Trace(ofstream *file) = 0;  

  //BTX
  // Description:
  // Called by Reset() if PVSource is set.
  virtual void ResetInternal() {};
  //ETX

  // Description:
  // Set/get the SM property to use with this widget..
  vtkSMProperty* GetSMProperty();
  void SetSMProperty(vtkSMProperty* prop);

  // Description:
  // Need access to these so that container-type widgets can set the property
  // name on the widgets they contain (e.g., vtkPVPointSourceWidget).
  vtkSetStringMacro(SMPropertyName);
  vtkGetStringMacro(SMPropertyName);

  vtkSetMacro(UseWidgetRange, int);
  vtkGetMacro(UseWidgetRange, int);
  vtkSetVector2Macro(WidgetRange, double);
  vtkGetVector2Macro(WidgetRange, double);

  // Description:
  // If HideGUI is true, the widget is not shown in the property page.
  vtkGetMacro(HideGUI, int);

  // Description:
  // If true, indicates that for the vtkPVReaderModule, this is a
  // widget that keeps the timestep. Note that a vtkPVReaderModule 
  // can have atmost one widget that keeps the timestep.
  vtkGetMacro(KeepsTimeStep, int);
protected:
  vtkPVWidget();
  ~vtkPVWidget();

  char *ModifiedCommandObjectTclName;
  char *ModifiedCommandMethod;
  vtkSetStringMacro(ModifiedCommandObjectTclName);
  vtkSetStringMacro(ModifiedCommandMethod);
  
  char *AcceptedCommandObjectTclName;
  char *AcceptedCommandMethod;
  vtkSetStringMacro(AcceptedCommandObjectTclName);
  vtkSetStringMacro(AcceptedCommandMethod);
  
  // This flag indicates that the widget has changed and should be
  // added to the trace file.
  int ModifiedFlag;


  // This flag indicates if the widget is a widget used to keep
  // timestep information for Readers.
  int KeepsTimeStep;

  vtkPVSource* PVSource;

  int UseWidgetRange;
  double WidgetRange[2];

  char* SMPropertyName;

//BTX
  virtual vtkPVWidget* ClonePrototypeInternal(vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);


  // Used void* instead of vtkPVWidget* to avoid reference counting
  vtkLinkedList<void*>* Dependents;
//ETX

  int HideGUI;

  vtkPVWidget* GetPVWidgetFromParser(vtkPVXMLElement* element,
                                     vtkPVXMLPackageParser* parser);
  vtkPVWindow* GetPVWindowFormParser(vtkPVXMLPackageParser* parser);


  // Saves for a specific part.  SaveInBatchScript loops over parts.
  // This is the way Accept and Reset should work.
  virtual void SaveInBatchScriptForPart(ofstream *file, 
                                        vtkClientServerID);

  //BTX
  friend class vtkPVWidgetObserver;
  vtkPVWidgetObserver* Observer;
  virtual void ExecuteEvent(vtkObject* obj, unsigned long event,
    void* calldata);
  //ETX

  void AddPropertyObservers(vtkSMProperty* property);
  void RemovePropertyObservers(vtkSMProperty* property);
private:
  vtkSMProperty* SMProperty;

  vtkPVWidget(const vtkPVWidget&); // Not implemented
  void operator=(const vtkPVWidget&); // Not implemented

};

#endif

