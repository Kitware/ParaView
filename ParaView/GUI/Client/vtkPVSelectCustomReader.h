/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectCustomReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSelectCustomReader - A super class for filter objects.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.
// This class should probably be merged with vtkPVComposite.
// Note when there are multiple outputs, a dummy pvsource has to
// be attached to each of those. This way, the user can add modules
// after each output.


#ifndef __vtkPVSelectCustomReader_h
#define __vtkPVSelectCustomReader_h

#include "vtkKWMessageDialog.h"

class vtkPVWindow;
class vtkPVReaderModule;

class VTK_EXPORT vtkPVSelectCustomReader : public vtkKWMessageDialog
{
public:
  static vtkPVSelectCustomReader* New();
  vtkTypeRevisionMacro(vtkPVSelectCustomReader,vtkKWMessageDialog);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkPVReaderModule* SelectReader(vtkPVWindow* win, const char*);
  
protected:
  vtkPVSelectCustomReader();
  ~vtkPVSelectCustomReader();

private:
  vtkPVSelectCustomReader(const vtkPVSelectCustomReader&); // Not implemented
  void operator=(const vtkPVSelectCustomReader&); // Not implemented
};

#endif
