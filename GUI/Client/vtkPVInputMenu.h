/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputMenu.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInputMenu menu for selecting input for a source.
// .SECTION Description
// This menu uses vtkPVSources as entries instead of vtkPVDatas.
// Use of the first input is hard coded.  If we want to allow more
// than one output, then this widget will have to be changed.

#ifndef __vtkPVInputMenu_h
#define __vtkPVInputMenu_h

#include "vtkPVWidget.h"

class vtkDataSet;
class vtkKWLabel;
class vtkKWOptionMenu;
class vtkPVData;
class vtkPVInputProperty;
class vtkPVSourceCollection;
class vtkSMInputProperty;

class VTK_EXPORT vtkPVInputMenu : public vtkPVWidget
{
public:
  static vtkPVInputMenu* New();
  vtkTypeRevisionMacro(vtkPVInputMenu, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set the label.  The label can be used to get this widget
  // from a script.
  void SetLabel (const char* label);
  const char* GetLabel();

  //BTX
  // Description:
  // This is the source collection as possible inputs.
  // If the collection gets modified, it will be reflected
  // in the menu on the next Reset call.
  // The collection is not referce counted for fear of loops 
  // and memory leaks.  We may wnet to fix his later.
  void SetSources(vtkPVSourceCollection *sources);
  vtkPVSourceCollection *GetSources();
  //ETX

  // Description:
  // This method is called when the source that contains this widget
  // is selected. Updates the menu in case more sources have been
  // created
  virtual void Select();
  
  int GetNumberOfSources();
  vtkPVSource* GetSource(int i);
  
  // Description:
  // The input name is usually "Input", but can be something else
  // (i.e. Source). Used to format commands in accept and reset methods.
  vtkSetStringMacro(InputName);
  
  // Description:
  // This method gets called when the user changes the widgets value,
  // or a script changes the widgets value.  Ideally, this method should 
  // be protected.  Input menus have the specifie behaviour that
  // the widgets Accept method is called when ever the input menu is changed.
  virtual void ModifiedCallback();

  // Description:
  // Set the menus value as a string.
  // Used by the Accept and Reset callbacks.
  void SetCurrentValue(vtkPVSource *pvs);
  vtkPVSource* GetCurrentValue() { return this->CurrentValue;}
  vtkPVSource* GetLastAcceptedValue();
  
  // Description:
  // Menu callback when an item is selected.
  void MenuEntryCallback(vtkPVSource *pvs);

  // Description:
  // Save this widget to a file.  
  virtual void SaveInBatchScript(ofstream *file);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVInputMenu* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  //BTX
  // Description:
  // Gets called when the accept button is pressed.
  // This method may add an entry to the trace file.
  virtual void Accept();
  //ETX

  // Description:
  // Initializes the input.
  virtual void Initialize();

  // Description:
  // Gets called when the reset button is pressed.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
  // Description:
  // The methods get called when reset is called.  
  // It can also get called on its own.  If the widget has options 
  // or configuration values dependent on the VTK object, this method
  // set these configuation object using the VTK object.
  virtual void Update();

protected:
  vtkPVInputMenu();
  ~vtkPVInputMenu();

  int InitializeWithCurrent;

  char* InputName;
  
  vtkPVSource *CurrentValue;
  vtkPVSourceCollection *Sources;
  
  vtkKWLabel *Label;
  vtkKWOptionMenu *Menu;

  // Description:
  // Reset the menu by taking all entries out.
  void DeleteAllEntries();

  // Description:
  // Add an entry to the menu.
  // If the source is not the right type, then this does nothing.
  // Returns 1 if a new entry was created, 0 if not the right type.
  int AddEntry(vtkPVSource *pvs);

  // Description:
  // When setting input, we have to be carefull not to cause the
  // loop. This code will determine if the operation would cause the
  // loop.
  int CheckForLoop(vtkPVSource *pvs);

  // Description:
  // This converts the InputName into an index.
  // SetSource -> SetNthPVInput(idx, ...);
  int GetPVInputIndex();

  // Description:
  // Get the property for this input.
  vtkSMInputProperty* GetInputProperty();

  // Description:
  // Adds a collection of sources to the menu.
  // The sources are filtered by "InputType".
  void AddSources(vtkPVSourceCollection *sources);

  vtkPVInputMenu(const vtkPVInputMenu&); // Not implemented
  void operator=(const vtkPVInputMenu&); // Not implemented

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
