/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInitialize.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInitialize - A super class for filter objects.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.
// This class should probably be merged with vtkPVComposite.
// Note when there are multiple outputs, a dummy pvsource has to
// be attached to each of those. This way, the user can add modules
// after each output.


#ifndef __vtkPVInitialize_h
#define __vtkPVInitialize_h

#include "vtkKWObject.h"

class vtkPVWindow;

class VTK_EXPORT vtkPVInitialize : public vtkKWObject
{
public:
  static vtkPVInitialize* New();
  vtkTypeRevisionMacro(vtkPVInitialize,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void Initialize(vtkPVWindow*);

protected:
  vtkPVInitialize();
  ~vtkPVInitialize();

  char* GetStandardFiltersInterfaces();
  char* GetStandardManipulatorsInterfaces();
  char* GetStandardReadersInterfaces();
  char* GetStandardSourcesInterfaces();
  char* GetStandardWritersInterfaces();

  char* StandardFiltersString;
  char* StandardManipulatorsString;
  char* StandardReadersString;
  char* StandardSourcesString;
  char* StandardWritersString;

  vtkSetStringMacro(StandardFiltersString);
  vtkSetStringMacro(StandardManipulatorsString);
  vtkSetStringMacro(StandardReadersString);
  vtkSetStringMacro(StandardSourcesString);
  vtkSetStringMacro(StandardWritersString);

private:
  vtkPVInitialize(const vtkPVInitialize&); // Not implemented
  void operator=(const vtkPVInitialize&); // Not implemented
};

#endif
