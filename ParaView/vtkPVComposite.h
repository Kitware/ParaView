/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVComposite.h
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
// .NAME vtkPVComposite - parallel filter/data object.
// .SECTION Description
// This composite is a parallel object.  It needs to be cloned
// in all the processes to work correctly.  After cloning, the parallel
// nature of the object is transparent.


#ifndef __vtkPVComposite_h
#define __vtkPVComposite_h

#include "vtkKWComposite.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"

class vtkPVWindow;

class VTK_EXPORT vtkPVComposite : public vtkKWComposite
{
public:
  static vtkPVComposite* New();
  vtkTypeMacro(vtkPVComposite,vtkKWComposite);
  
  // Description:
  // This duplicates the object in the satellite processes.
  // They will all have the same tcl name.
  void Clone(vtkPVApplication *app);
  
  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties(char *args);

  void SetPropertiesParent(vtkKWWidget *parent);
  vtkKWWidget *GetPropertiesParent();
  vtkKWWidget *GetProperties();

  // Description:
  // Get the Prop for this class.
  virtual vtkProp *GetProp();

  // Description:
  // A convenience method that returns the data from the source.
  vtkPVData *GetPVData();

  // Description:
  // The source should really be merged with this composite class.
  // The set method assigns the source in all of the processes.
  void SetSource(vtkPVSource *source);
  vtkGetObjectMacro(Source, vtkPVSource);

  void Select(vtkKWView *);
  void Deselect(vtkKWView *);

  void SetWindow(vtkPVWindow *window);
  vtkPVWindow *GetWindow();
  
  // Description:
  // This name is used in the data list to identify the composite.
  virtual void SetCompositeName(const char *name);
  char* GetCompositeName();
  
  // Description:
  // This flage turns the visibility of the prop on and off.  These methods transmit
  // the state change to all of the satellite processes.
  void SetVisibility(int v);
  int GetVisibility();
  vtkBooleanMacro(Visibility, int);

  // Description:
  // This just returns the application typecast correctly.
  vtkPVApplication* GetPVApplication();

protected:
  vtkPVComposite();
  ~vtkPVComposite();
  
  vtkKWNotebook *Notebook;
  vtkPVSource *Source;
  vtkPVWindow *Window;
  
  int NotebookCreated;
  char *CompositeName;
};

#endif
