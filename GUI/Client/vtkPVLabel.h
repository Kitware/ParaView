/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLabel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLabel -
// .SECTION Description

#ifndef __vtkPVLabel_h
#define __vtkPVLabel_h

#include "vtkPVObjectWidget.h"

class vtkKWApplication;
class vtkKWLabel;

class VTK_EXPORT vtkPVLabel : public vtkPVObjectWidget
{
public:
  static vtkPVLabel* New();
  vtkTypeRevisionMacro(vtkPVLabel, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Setting the label also sets the name.
  void SetLabel(const char *str);
  const char* GetLabel();

  virtual void Create(vtkKWApplication *pvApp);
    
  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to a widget it contains.
  virtual void SetBalloonHelpString(const char *str);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVLabel* ClonePrototype(vtkPVSource* pvSource,
                             vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // Empty method to keep superclass from complaining.
  virtual void Trace(ofstream *) {};

  // Description:
  // Save this widget to a file.
  virtual void SaveInBatchScript(ofstream *) {};

protected:
  vtkPVLabel();
  ~vtkPVLabel();
  
  vtkKWLabel *Label;

  vtkPVLabel(const vtkPVLabel&); // Not implemented
  void operator=(const vtkPVLabel&); // Not implemented

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
