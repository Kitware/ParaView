/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataList.h
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
// .NAME vtkPVDataList
// .SECTION Description
// This is the UI for the Assembly browser / editor.

#ifndef __vtkPVDataList_h
#define __vtkPVDataList_h

#include "vtkKWWidget.h"
#include "vtkPVComposite.h"

class vtkKWEntry;

class VTK_EXPORT vtkPVDataList : public vtkKWWidget
{
public:
  static vtkPVDataList* New() {return new vtkPVDataList;};
  vtkTypeMacro(vtkPVDataList,vtkKWWidget);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // The assembly that is displayed in the editor.
  vtkGetObjectMacro(CompositeCollection, vtkCollection);

  // Description:
  // Redraws the canvas (assembly or list changed).
  void Update();

  // Description:
  // Callback from the canvas buttons.
  void Pick(int assyIdx);
  void ToggleVisibility(int assyIdx, int button);
  void EditColor(int assyIdx);

  // Description:
  // Callbacks from menu items.
  void DeletePicked();
  void DeletePickedVerify();

  // Description:
  // This method gets called when the user clicks on the assy.
  // It puts the name entry in the correct position of the canvas and waits.
  void EditName(int assyIdx);

  // Description:
  // This method gets called when the user presses return in the name entry.
  void NameEntryClose();

protected:
  vtkPVDataList();
  ~vtkPVDataList();
  vtkPVDataList(const vtkPVDataList&) {};
  void operator=(const vtkPVDataList&) {};

  int Update(vtkGoAssembly *assy, int y, int in, int pickable);

  vtkKWWidget *ScrollFrame;
  vtkKWWidget *Canvas;
  vtkKWWidget *ScrollBar;

  
  vtkCollection *CompositeCollection;

  // -- Stuff for entering the name in the canvas --
  vtkKWEntry *NameEntry;
  vtkPVComposite *NameEntryComposite;
  char *NameEntryTag;

  vtkSetObjectMacro(NameEntryComposite, vtkPVComposite);
  vtkSetStringMacro(NameEntryTag);

};

#endif


