/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAdvancedReaderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAdvancedReaderModule - Module representing an "advanced" reader
// .SECTION Description
// The class vtkPVAdvancedReaderModule is used to represent an "advanced"
// reader (or a pipeline which contains a reader). An advanced reader is
// one which allows the user to pre-select certain attributes (for example,
// list of arrays to be loaded) before reading the whole file.  This is
// done by reading some header information during UpdateInformation.  The
// main difference between vtkPVAdvancedReaderModule and vtkPVReaderModule
// is that the former does not automatically call Accept after the filename
// is selected. Instead, it prompts the user for more selections. The file
// is only fully loaded when the user presses Accept.
//
// .SECTION See also
// vtkPVReadermodule vtkPVEnSightReaderModule vtkPVPLOT3DReaderModule



#ifndef __vtkPVAdvancedReaderModule_h
#define __vtkPVAdvancedReaderModule_h

#include "vtkPVReaderModule.h"

class VTK_EXPORT vtkPVAdvancedReaderModule : public vtkPVReaderModule
{
public:
  static vtkPVAdvancedReaderModule* New();
  vtkTypeRevisionMacro(vtkPVAdvancedReaderModule, vtkPVReaderModule);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Used mainly by the scripting interface, these three methods are
  // normally called in order during the file opening process. Given
  // the reader module name, InitializeReadCustom() returns a clone
  // which can be passed to ReadFileInformation() and FinalizeRead()
  // to finish the reading process.
  virtual int Initialize(const char* fname, vtkPVReaderModule*& newModule);
  virtual int Finalize  (const char* fname);
  virtual int ReadFileInformation(const char* fname);

protected:
  vtkPVAdvancedReaderModule();
  ~vtkPVAdvancedReaderModule();

private:
  vtkPVAdvancedReaderModule(const vtkPVAdvancedReaderModule&); // Not implemented
  void operator=(const vtkPVAdvancedReaderModule&); // Not implemented
};

#endif
