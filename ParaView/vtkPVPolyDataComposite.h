/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataComposite.h
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

#ifndef __vtkPVPolyDataComposite_h
#define __vtkPVPolyDataComposite_h

#include "vtkKWComposite.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkKWLabel.h"
#include "vtkPVPolyData.h"
#include "vtkPVSource.h"

class vtkPVWindow;

class VTK_EXPORT vtkPVPolyDataComposite : public vtkKWComposite
{
public:
  static vtkPVPolyDataComposite* New();
  
  // Description:
  // Get the Prop for this class.
  virtual vtkProp *GetProp();

  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties(vtkKWApplication *app, char *args);
  
  void SetPropertiesParent(vtkKWWidget *parent);
  vtkKWWidget *GetPropertiesParent();
  vtkKWWidget *GetProperties();
  
  void Select(vtkKWView *);
  void Deselect(vtkKWView *);
  
  vtkPVPolyDataComposite *GetComposite();

  void SetTabLabels(char *label1, char *label2);

  void SetWindow(vtkPVWindow *window);
  vtkPVWindow *GetWindow();

  vtkSetObjectMacro(PVPolyData, vtkPVPolyData);
  vtkGetObjectMacro(PVPolyData, vtkPVPolyData);
  
  vtkSetObjectMacro(PVSource, vtkPVSource);
  vtkGetObjectMacro(PVSource, vtkPVSource);
protected:
  vtkPVPolyDataComposite();
  ~vtkPVPolyDataComposite();
  
  vtkKWNotebook *Notebook;
  vtkPVPolyData *PVPolyData;
  vtkPVSource *PVSource;
  vtkActor *Actor;
  
  char *Label1;
  char *Label2;
  
  int NotebookCreated;
  
  vtkPVWindow *Window;
};

#endif
