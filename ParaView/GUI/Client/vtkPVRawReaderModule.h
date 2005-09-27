/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRawReaderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRawReaderModule -
// .SECTION Description
//
// .SECTION See also
// vtkPVAdvancedReaderModule


#ifndef __vtkPVRawReaderModule_h
#define __vtkPVRawReaderModule_h

#include "vtkPVAdvancedReaderModule.h"

class VTK_EXPORT vtkPVRawReaderModule : public vtkPVAdvancedReaderModule
{
public:
  static vtkPVRawReaderModule* New();
  vtkTypeRevisionMacro(vtkPVRawReaderModule, vtkPVAdvancedReaderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set up the UI for this source. The only default element created is a
  // file entry. Of course, more can be added in the configuration file.
  virtual void CreateProperties();
protected:
  vtkPVRawReaderModule();
  ~vtkPVRawReaderModule();

private:
  vtkPVRawReaderModule(const vtkPVRawReaderModule&); // Not implemented
  void operator=(const vtkPVRawReaderModule&); // Not implemented
};

#endif
