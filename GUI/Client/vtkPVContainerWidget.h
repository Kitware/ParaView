/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContainerWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVContainerWidget -
// .SECTION Description


#ifndef __vtkPVContainerWidget_h
#define __vtkPVContainerWidget_h

#include "vtkPVWidget.h"

//BTX
template <class key, class data> 
class vtkArrayMap;
class vtkPVWidgetCollection;
//ETX

class VTK_EXPORT vtkPVContainerWidget : public vtkPVWidget
{
public:
  static vtkPVContainerWidget* New();
  vtkTypeRevisionMacro(vtkPVContainerWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // Creates common widgets.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Add widgets to the possible selection.  The vtkValue
  // is value used to set the vtk object variable.
  void AddPVWidget(vtkPVWidget *pvw);

  // Description:
  // This method considers all contained widgets 
  // when computing the modified flag.
  virtual int GetModifiedFlag();  

  // Description:
  // This method is called when the source that contains this widget
  // is selected.
  void Select();

  // Description:
  // This method is called when the source that contains this widget
  // is deselected.
  void Deselect();

  // Description:
  // Return ith widget.
  vtkPVWidget* GetPVWidget(vtkIdType i);

  // Description:
  // Return the widget with the corresponding trace name.
  vtkPVWidget* GetPVWidget(const char* traceName);

  // Description:
  // The direction in which the sub-widgets are packed
  // ( top, left etc. )
  vtkSetStringMacro(PackDirection);
  vtkGetStringMacro(PackDirection);
    
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVContainerWidget* ClonePrototype(vtkPVSource* pvSource,
                                    vtkArrayMap<vtkPVWidget*, 
                                    vtkPVWidget*>* map);
//ETX

  // Description:
  // For saving the widget into a VTK tcl script.
  virtual void SaveInBatchScript(ofstream *file);

  //BTX
  // Description:
  // Called when accept button is pushed.
  // Adds to the trace file and sets the objects variable from UI.
  virtual void Accept();
  virtual void PostAccept();
  //ETX

  // Description:
  // Called when reset button is pushed.
  // Sets UI current value from objects variable.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // adds a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu(vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai);
 
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkPVContainerWidget();
  ~vtkPVContainerWidget();

//BTX

  vtkPVWidgetCollection *Widgets;

  virtual vtkPVWidget* ClonePrototypeInternal(vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  char* PackDirection;

private:
  vtkPVContainerWidget(const vtkPVContainerWidget&); // Not implemented
  void operator=(const vtkPVContainerWidget&); // Not implemented
};

#endif
