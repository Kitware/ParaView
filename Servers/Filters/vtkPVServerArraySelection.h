/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerArraySelection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerArraySelection - Server-side helper for vtkPVArraySelection.
// .SECTION Description

#ifndef __vtkPVServerArraySelection_h
#define __vtkPVServerArraySelection_h

#include "vtkPVServerObject.h"

class vtkClientServerStream;
class vtkPVServerArraySelectionInternals;
class vtkAlgorithm;

class VTK_EXPORT vtkPVServerArraySelection : public vtkPVServerObject
{
public:
  static vtkPVServerArraySelection* New();
  vtkTypeMacro(vtkPVServerArraySelection, vtkPVServerObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get a list of array names that can be read by the given reader
  // object.  The second argument has to be the name of array.
  const vtkClientServerStream& GetArraySettings(vtkAlgorithm*, const char* arrayname);

protected:
  vtkPVServerArraySelection();
  ~vtkPVServerArraySelection();

  // Internal implementation details.
  vtkPVServerArraySelectionInternals* Internal;
private:
  vtkPVServerArraySelection(const vtkPVServerArraySelection&); // Not implemented
  void operator=(const vtkPVServerArraySelection&); // Not implemented
};

#endif
