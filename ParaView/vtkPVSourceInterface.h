/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSourceInterface.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkPVSourceInterface - Everything PV needs to make a UI for a filter.
// .SECTION Description
// This contains every thing PV needs to create a UI for a VTK source.

#ifndef __vtkPVSourceInterface_h
#define __vtkPVSourceInterface_h

#include "vtkKWObject.h"
#include "vtkPVApplication.h"
#include "vtkPVMethodInterface.h"
#include "vtkPVWindow.h"
#include "vtkCollection.h"

class VTK_EXPORT vtkPVSourceInterface : public vtkKWObject
{
public:
  static vtkPVSourceInterface* New();
  vtkTypeMacro(vtkPVSourceInterface, vtkKWObject);
  
  // Description:
  // The name of the vtk object created.
  vtkSetStringMacro(SourceClassName);
  vtkGetStringMacro(SourceClassName);
  
  // Description:
  // Name (with instance num) displayed in the UI.
  vtkSetStringMacro(RootName);
  vtkGetStringMacro(RootName);
  
  // Description:
  // For now we are supporting only one input and output.
  vtkSetStringMacro(InputClassName);
  vtkGetStringMacro(InputClassName);
  vtkSetStringMacro(OutputClassName);
  vtkGetStringMacro(OutputClassName);
  
  // Description:
  // Set/Get name of input file.
  vtkSetStringMacro(DataFileName);
  vtkGetStringMacro(DataFileName);
  
  // Description:
  // Does this filter operate on scalars?
  vtkBooleanMacro(DefaultScalars, int);
  vtkGetMacro(DefaultScalars, int);
  vtkSetMacro(DefaultScalars, int);

  // Description:
  // Does this filter operate on vectors?
  vtkBooleanMacro(DefaultVectors, int);
  vtkGetMacro(DefaultVectors, int);
  vtkSetMacro(DefaultVectors, int);

  // Description:
  // Access to the method interfaces.
  void AddMethodInterface(vtkPVMethodInterface *methonInt);
  vtkCollection *GetMethodInterfaces() {return this->MethodInterfaces;}
  
  // Description:
  // Where to put any new source composites created.
  // This is here because the callback does not have an argument.
  // No reference counting.
  void SetPVWindow(vtkPVWindow *pvw);
  
  // Description:
  // This method is called by the window to determine if this filter should be
  // added to the filter menu.  Right now, only the class name of the input
  // is checked.  In the future, attributes could be checked as well.
  virtual int GetIsValidInput(vtkPVData *input);
  
  // Decription:
  // This method is called to create another source.
  vtkPVSource *CreateCallback();

  // Description:
  // Save this interface to a file.
  virtual void Save(ofstream *file, const char* sourceName);
  
protected:
  vtkPVSourceInterface();
  ~vtkPVSourceInterface();
  vtkPVSourceInterface(const vtkPVSourceInterface&) {};
  void operator=(const vtkPVSourceInterface&) {};

  char *RootName;
  char *SourceClassName;
  char *InputClassName;
  char *OutputClassName;
  char *DataFileName;
  
  int DefaultScalars;
  int DefaultVectors;
  
  vtkCollection *MethodInterfaces;
  
  // Extra stuff (not specific to interface.)
  vtkPVApplication *GetPVApplication();
  vtkPVWindow *PVWindow;
  int InstanceCount;
};

#endif
