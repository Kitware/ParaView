/*=========================================================================

  Program:   ParaView
  Module:    vtkPVObjectWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVObjectWidget - Widget the represents an objects variable.
// .SECTION Description
// vtkPVObjectWidget is a special class of vtkPVWidget that
// represents a VTK object's variable.  It has ivars for the VTK objects
// name and its variable name.  The Reset and Accept commands can format
// scripts from these variables.  The name of this class may not be the best.
// .NOTE
// Since we have created the AcceptInternal method that has the object tcl
// name as an argument, many classes do not need this superclass or
// use its ivar ObjectID.  I have not removed the class because I believe
// some widgets use this class when the object is a PV object and not a VTK object.
// I will have to clean this up later

//!!!!!!!!!!!!!!

#ifndef __vtkPVObjectWidget_h
#define __vtkPVObjectWidget_h

#include "vtkPVWidget.h"

class VTK_EXPORT vtkPVObjectWidget : public vtkPVWidget
{
public:
  vtkTypeRevisionMacro(vtkPVObjectWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // The point of a PV widget is that it is an interface for
  // some objects state/ivars.  This is one way the object/variable
  // can be specified. Subclasses may have seperate or addition
  // variables for specifying the relationship.
  vtkSetMacro(ObjectID,vtkClientServerID);
  //ETX
  vtkSetStringMacro(VariableName);
  vtkGetStringMacro(VariableName);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVObjectWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  // Description:
  // Get an object from a widget by name
  virtual vtkClientServerID GetObjectByName(const char*){ vtkClientServerID id = {0}; return id;}
//ETX

protected:
  vtkPVObjectWidget();
  ~vtkPVObjectWidget();

  vtkClientServerID ObjectID;
  char *VariableName;

  vtkPVObjectWidget(const vtkPVObjectWidget&); // Not implemented
  void operator=(const vtkPVObjectWidget&); // Not implemented
  
//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  // Description:
  // An interface for saving a widget into a script.
  virtual void SaveInBatchScriptForPart(ofstream *file, vtkClientServerID);
};

#endif
