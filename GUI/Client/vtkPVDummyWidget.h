/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDummyWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDummyWidget - an empty widget
// .SECTION Description
// This empty widget can be used to create containers/select widgets which
// require at least one widget without anything visible.

#ifndef __vtkPVDummyWidget_h
#define __vtkPVDummyWidget_h

#include "vtkPVWidget.h"

class VTK_EXPORT vtkPVDummyWidget : public vtkPVWidget
{
public:
  static vtkPVDummyWidget* New();
  vtkTypeRevisionMacro(vtkPVDummyWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVDummyWidget* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  //BTX
  // Description:
  // Trying out a ne protocal.
  virtual void Accept() { this->ModifiedFlag = 0; }
  virtual void ResetIntenral(vtkClientServerID) { this->ModifiedFlag = 0;}
  //ETX

  // Description:
  // Empty method to keep superclass from complaining.
  virtual void Trace(ofstream*) {};


  // Description:
  // This does nothing.  It is only here to avoid a paraview warning.
  virtual void SaveInBatchScript(ofstream*) {};

protected:
  vtkPVDummyWidget();
  ~vtkPVDummyWidget();


//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  vtkPVDummyWidget(const vtkPVDummyWidget&); // Not implemented
  void operator=(const vtkPVDummyWidget&); // Not implemented

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
