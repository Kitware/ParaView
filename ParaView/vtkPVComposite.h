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
// .NAME vtkPVComposite - an element in a view containing props / properties
// .SECTION Description
// A composite represents an element in the view. It is very similar to
// the notion of an actor in a renderer. The composite is different in 
// that it combines the Actor (vtkProp actually) with some user interface
// code called the properties, and it may also contain more complex
// elements such as filters etc.

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
  
  // Description:
  // Get the Prop for this class.
  virtual vtkProp *GetProp();

  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties(vtkKWApplication *app, char *args);

  void SetPropertiesParent(vtkKWWidget *parent);
  vtkKWWidget *GetPropertiesParent();
  vtkKWWidget *GetProperties();

  void SetData(vtkPVData *data);
  vtkPVData *GetData();

  void SetSource(vtkPVSource *source);
  vtkGetObjectMacro(Source, vtkPVSource);

  void Select(vtkKWView *);
  void Deselect(vtkKWView *);

  void SetWindow(vtkPVWindow *window);
  vtkPVWindow *GetWindow();
  
  virtual void SetCompositeName(const char *name);
  char* GetCompositeName();
  
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
