/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSource.h
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
// .NAME vtkPVSource - A super class for filter objects.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.
// This class should probably be merged with vtkPVComposite.


#ifndef __vtkPVSource_h
#define __vtkPVSource_h

#include "vtkKWLabel.h"
#include "vtkPVData.h"
#include "vtkSource.h"
#include "vtkKWComposite.h"
class vtkPVWindow;


class VTK_EXPORT vtkPVSource : public vtkKWComposite
{
public:
  static vtkPVSource* New();
  vtkTypeMacro(vtkPVSource,vtkKWComposite);
  
  // Description:
  // This duplicates the object in the satellite processes.
  // They will all have the same tcl name.
  void Clone(vtkPVApplication *app);
    
  // Description:
  // Get the Window for this class.
  vtkPVWindow *GetWindow();
  
  // Description:
  // A way to get the output in the superclass.
  vtkPVData *GetPVData() {return this->Output;}
    
  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties();
  virtual void ShowProperties();

  void CreateDataPage();
  
  // Description:
  // Methods to indicate when this composite is the selected composite.
  // These methods are used by subclasses to modify the menu bar
  // for example. When a volume composite is selected it might 
  // add an option to the menu bar to view the 2D slices.
  virtual void Select(vtkKWView *);
  virtual void Deselect(vtkKWView *);

  // Description:
  // This flage turns the visibility of the prop on and off.  These methods transmit
  // the state change to all of the satellite processes.
  void SetVisibility(int v);
  int GetVisibility();
  vtkBooleanMacro(Visibility, int);

  // Description:
  // This name is used in the data list to identify the composite.
  virtual void SetName(const char *name);
  char* GetName();

  vtkGetObjectMacro(Input, vtkPVData);
  
  // Description:
  // This just returns the application typecast correctly.
  vtkPVApplication* GetPVApplication();  

  // Description:
  // Sources have no props.
  vtkProp *GetProp() {return NULL;}
  
protected:
  vtkPVSource();
  ~vtkPVSource();
  vtkPVSource(const vtkPVSource&) {};
  void operator=(const vtkPVSource&) {};
  
  // Description:
  // Mangages the double pointer and reference counting.
  void SetPVData(vtkPVData *data);
  
  vtkPVData *Output;
  
  // Just one input for now.
  vtkPVData *Input;
  char *Name;
  vtkKWWidget *Properties;
  
  int DataCreated;
};

#endif


