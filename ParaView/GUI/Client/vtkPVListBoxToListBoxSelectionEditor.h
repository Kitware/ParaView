/*=========================================================================

  Module:    vtkPVListBoxToListBoxSelectionEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVListBoxToListBoxSelectionEditor - provides API to add a set
// of strings at once.
// .SECTION Description
// vtkKWListBoxToListBoxSelectionEditor uses AppendUnique() and like which
// is very expensive for addition/moving of items from one list to other.
// This class overcomes that by allowing caller to set the entire lists and
// checking for mutual exclusions using stl sets. Hence, is much better suited 
// for use in vtkPVFileEntry where there could easily be large number of files.

#ifndef __vtkPVListBoxToListBoxSelectionEditor_h
#define __vtkPVListBoxToListBoxSelectionEditor_h

#include "vtkKWListBoxToListBoxSelectionEditor.h"


class vtkStringList;

class VTK_EXPORT vtkPVListBoxToListBoxSelectionEditor : 
  public vtkKWListBoxToListBoxSelectionEditor
{
public:
  static vtkPVListBoxToListBoxSelectionEditor* New();
  vtkTypeRevisionMacro(vtkPVListBoxToListBoxSelectionEditor, 
    vtkKWListBoxToListBoxSelectionEditor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the source list. Clears any items already present in the list.
  // If force==0, then only those items not in the final list are added.
  // If force==1, then all items in list are added however, those also
  // present in final list are removed from the final list.
  void SetSourceList(vtkStringList* list, int force = 0);

  // Description:
  // Set the final list.  Clears any items already present in the list.
  // If force==0, then only those items not in the source list are added.
  // If force==1, then all items in list are added however, those also
  // present in source list are removed from the source list.
  void SetFinalList(vtkStringList* list, int force = 0);

protected:
  vtkPVListBoxToListBoxSelectionEditor();
  ~vtkPVListBoxToListBoxSelectionEditor();
 
  // Internal method.
  void SetList(vtkStringList* list, vtkKWListBox* toAdd, 
    vtkKWListBox* toComp, int force);
  
  // Description:
  // Overridden to overcome use of vtkKWListBox::AppendUnique(). 
  virtual void MoveWholeList(vtkKWListBox* l1, vtkKWListBox* l2);

private:
  vtkPVListBoxToListBoxSelectionEditor(const vtkPVListBoxToListBoxSelectionEditor&); // Not implemented.
  void operator=(const vtkPVListBoxToListBoxSelectionEditor&); // Not implemented.
};

#endif
