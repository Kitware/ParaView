/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourceList.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSourceList
// .SECTION Description
// This is the UI for the Assembly browser / editor.

#ifndef __vtkPVSourceList_h
#define __vtkPVSourceList_h

#include "vtkPVSourcesNavigationWindow.h"

class vtkKWEntry;
class vtkPVSource;
class vtkPVSourceCollection;
class vtkKWMenu;

class VTK_EXPORT vtkPVSourceList : public vtkPVSourcesNavigationWindow
{
public:
  static vtkPVSourceList* New();
  vtkTypeRevisionMacro(vtkPVSourceList,vtkPVSourcesNavigationWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Callback from the canvas buttons.
  void Pick(int assyIdx);
  void ToggleVisibility(int assyIdx, char* id, int button);
  void EditColor(int assyIdx);

  // Description:
  // This method is called before the object is deleted.
  virtual void PrepareForDelete();
 
protected:
  vtkPVSourceList();
  ~vtkPVSourceList();

  virtual void ChildUpdate(vtkPVSource*);
  virtual void PostChildUpdate();

  // Description:
  // Create a Tk widget
  virtual void ChildCreate();

  int UpdateSource(vtkPVSource *comp, int y, int in, int current);
  void UpdateVisibility(vtkPVSource *comp, const char *id);

  // Description:
  // The assembly that is displayed in the editor.
  vtkGetObjectMacro(Sources, vtkPVSourceCollection);
  virtual void SetSources(vtkPVSourceCollection*);
  vtkPVSourceCollection *Sources;

  int StartY;
  int LastY;
  int CurrentY;

private:
  vtkPVSourceList(const vtkPVSourceList&); // Not implemented
  void operator=(const vtkPVSourceList&); // Not implemented
};

#endif


