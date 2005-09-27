/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightReaderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVEnSightReaderModule - Module representing an "advanced" reader
// .SECTION Description
// The class vtkPVEnSightReaderModule is used to represent an "advanced"
// reader (or a pipeline which contains a reader). An advanced reader is
// one which allows the user to pre-select certain attributes (for example,
// list of arrays to be loaded) before reading the whole file.  This is
// done by reading some header information during UpdateInformation.  The
// main difference between vtkPVEnSightReaderModule and vtkPVReaderModule
// is that the former does not automatically call Accept after the filename
// is selected. Instead, it prompts the user for more selections. The file
// is only fully loaded when the user presses Accept.
//
// .SECTION See also
// vtkPVReadermodule vtkPVEnSightReaderModule vtkPVPLOT3DReaderModule

#ifndef __vtkPVEnSightReaderModule_h
#define __vtkPVEnSightReaderModule_h

#include "vtkPVAdvancedReaderModule.h"

class VTK_EXPORT vtkPVEnSightReaderModule : public vtkPVAdvancedReaderModule
{
public:
  static vtkPVEnSightReaderModule* New();
  vtkTypeRevisionMacro(vtkPVEnSightReaderModule, vtkPVAdvancedReaderModule);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // If the source is a vtkPVEnSightMasterServerReader, this sets the
  // controller.  In either case, the superclass's implementation of
  // this method is called.
  virtual int ReadFileInformation(const char* fname);

  // Description:
  // Save the pipeline to a batch file which can be run without
  // a user interface.
  virtual void SaveInBatchScript(ofstream *file);
 
protected:
  vtkPVEnSightReaderModule();
  ~vtkPVEnSightReaderModule();

  // This method is called when the accept button is pressed for the
  // first time.
  virtual int InitializeData();
  virtual void CreateProperties();
 
private:
  vtkPVEnSightReaderModule(const vtkPVEnSightReaderModule&); // Not implemented
  void operator=(const vtkPVEnSightReaderModule&); // Not implemented
};

#endif
