/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataSetReaderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDataSetReaderModule - A class to handle the UI for vtkGlyph3D
// .SECTION Description


#ifndef __vtkPVDataSetReaderModule_h
#define __vtkPVDataSetReaderModule_h

#include "vtkPVReaderModule.h"

class VTK_EXPORT vtkPVDataSetReaderModule : public vtkPVReaderModule
{
public:
  static vtkPVDataSetReaderModule* New();
  vtkTypeRevisionMacro(vtkPVDataSetReaderModule, vtkPVReaderModule);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // Create the PVDataSetReaderModule widgets.
  virtual void InitializePrototype();

  // Description:
  virtual int Initialize(const char* fname, vtkPVReaderModule*& prm);

protected:
  vtkPVDataSetReaderModule();
  ~vtkPVDataSetReaderModule();

private:
  vtkPVDataSetReaderModule(const vtkPVDataSetReaderModule&); // Not implemented
  void operator=(const vtkPVDataSetReaderModule&); // Not implemented
};

#endif
