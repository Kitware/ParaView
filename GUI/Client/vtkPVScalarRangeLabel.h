/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScalarRangeLabel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVScalarRangeLabel - Shows the scalar range of and array.
// .SECTION Description
// This label gets an array from an array menu, and shows its scalar range.
// It shows nothing right now if the array has more than one component.


#ifndef __vtkPVScalarRangeLabel_h
#define __vtkPVScalarRangeLabel_h

#include "vtkPVWidget.h"

class vtkKWApplication;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkPVArrayMenu;

class VTK_EXPORT vtkPVScalarRangeLabel : public vtkPVWidget
{
public:
  static vtkPVScalarRangeLabel* New();
  vtkTypeRevisionMacro(vtkPVScalarRangeLabel, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);

  // Description:
  // The scalar range display gets its array object from the array menu.
  virtual void SetArrayMenu(vtkPVArrayMenu*);
  vtkGetObjectMacro(ArrayMenu, vtkPVArrayMenu);

  // Description:
  // This calculates new range to display (using the array menu).
  virtual void Update();

  // Description:
  // Access to the range values.  This is used in a regression test.
  vtkGetVector2Macro(Range, double);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create a new
  // instance of the same type as the current object using
  // NewInstance() and then copy some necessary state parameters.
  vtkPVScalarRangeLabel* ClonePrototype(vtkPVSource* pvSource,
                                        vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Empty method to keep superclass from complaining.
  virtual void SaveInBatchScript(ofstream*) {};

  // Description:
  // Empty method to keep superclass from complaining.
  virtual void Trace(ofstream*) {};

  // Description:
  // This widget does not actually use Accept, but it has to override the
  // pure virtual method of the superclass.
  virtual void Accept() {this->Superclass::Accept();}

protected:
  vtkPVScalarRangeLabel();
  ~vtkPVScalarRangeLabel();

  vtkPVArrayMenu *ArrayMenu;
  vtkKWLabel *Label;

  double Range[2];


//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVScalarRangeLabel(const vtkPVScalarRangeLabel&); // Not implemented
  void operator=(const vtkPVScalarRangeLabel&); // Not implemented
};


#endif
