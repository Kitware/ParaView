/*=========================================================================

  Program:   ParaView
  Module:    vtkXDMFReaderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXDMFReaderModule - Module representing an "advanced" reader
// .SECTION Description
// The class vtkXDMFReaderModule is used to represent an "advanced"
// reader (or a pipeline which contains a reader). An advanced reader is
// one which allows the user to pre-select certain attributes (for example,
// list of arrays to be loaded) before reading the whole file.  This is
// done by reading some header information during UpdateInformation.  The
// main difference between vtkXDMFReaderModule and vtkPVReaderModule
// is that the former does not automatically call Accept after the filename
// is selected. Instead, it prompts the user for more selections. The file
// is only fully loaded when the user presses Accept.
//
// .SECTION See also
// vtkPVReadermodule vtkPVEnSightReaderModule



#ifndef __vtkXDMFReaderModule_h
#define __vtkXDMFReaderModule_h

#include "vtkPVAdvancedReaderModule.h"

class vtkKWOptionMenu;
class vtkKWListBox;
class vtkXDMFReaderModuleInternal;
class vtkKWFrameLabeled;

class VTK_EXPORT vtkXDMFReaderModule : public vtkPVAdvancedReaderModule
{
public:
  static vtkXDMFReaderModule* New();
  vtkTypeRevisionMacro(vtkXDMFReaderModule, vtkPVAdvancedReaderModule);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Tries to read a given file. Return VTK_OK on success, VTK_ERROR
  // on failure. A new instance of a reader module (which contains the 
  // actual VTK reader to be used) is returned. This should be called
  // only on a prototype.
  virtual int Initialize(const char* fname, vtkPVReaderModule*& prm);
  virtual int ReadFileInformation(const char* fname);
  virtual int Finalize(const char* fname);

  // Description:
  // Saves the pipeline in a ParaView script.  This is similar
  // to saveing a trace, except only the last state is stored.
  virtual void SaveState(ofstream *file);

  vtkSetStringMacro(Domain);

  void UpdateGrids();
  void UpdateDomains();

  void EnableGrid(const char* grid);
  void EnableAllGrids();

  // Description:
  // Save the pipeline to a batch file which can be run without
  // a user interface.
  virtual void SaveInBatchScript(ofstream *file);

protected:
  vtkXDMFReaderModule();
  ~vtkXDMFReaderModule();

  vtkKWFrameLabeled *DomainGridFrame;
  vtkKWOptionMenu *DomainMenu;
  vtkKWListBox* GridSelection;

  char *Domain;
  vtkXDMFReaderModuleInternal* Internals;

private:
  vtkXDMFReaderModule(const vtkXDMFReaderModule&); // Not implemented
  void operator=(const vtkXDMFReaderModule&); // Not implemented
};

#endif
