/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightMasterServerReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVEnSightMasterServerReader - 
// .SECTION Description

#ifndef __vtkPVEnSightMasterServerReader_h
#define __vtkPVEnSightMasterServerReader_h

#include "vtkGenericEnSightReader.h"

class vtkMultiProcessController;
class vtkPVEnSightMasterServerReaderInternal;
class vtkPVEnSightMasterServerTranslator;

class VTK_EXPORT vtkPVEnSightMasterServerReader : public vtkGenericEnSightReader
{
public:
  static vtkPVEnSightMasterServerReader* New();
  vtkTypeRevisionMacro(vtkPVEnSightMasterServerReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This class uses MPI communication mechanisms to verify the
  // integrity of all case files in the master file.  The get method
  // interface must use vtkMultiProcessController instead of
  // vtkMPIController because Tcl wrapping requires the class's
  // wrapper to be defined, but it is not defined if MPI is not on.
  // In client-server mode, we may still need to create an instance of
  // this class on the client process even if MPI is not compiled in.
  virtual vtkMultiProcessController* GetController();
  virtual void SetController(vtkMultiProcessController* controller);

  // Description:
  // Return whether we can read the file given.
  virtual int CanReadFile(const char*);
  
  // Description:
  // Get the number of pieces in the file.  Valid after
  // UpdateInformation.
  vtkGetMacro(NumberOfPieces, int);
protected:
  vtkPVEnSightMasterServerReader();
  ~vtkPVEnSightMasterServerReader();
  
  virtual void ExecuteInformation();
  virtual void Execute();
  int ParseMasterServerFile();
  
  void SuperclassExecuteInformation();
  void SuperclassExecuteData();
  
  // Special execute method called by Execute when an error has
  // occurred.  It produces an empty output.
  virtual void ExecuteError();
  
  // The MPI controller used to communicate with the instances in
  // other processes.
  vtkMultiProcessController* Controller;
  
  // The number of pieces available in the file.
  int NumberOfPieces;
  
  // Internal implementation details.
  vtkPVEnSightMasterServerReaderInternal* Internal;
  
  // The extent translator used to provide the correct breakdown of
  // pieces across processes.
  vtkPVEnSightMasterServerTranslator* ExtentTranslator;
  
  // Whether an error occurred during ExecuteInformation.
  int InformationError;
  
private:
  vtkPVEnSightMasterServerReader(const vtkPVEnSightMasterServerReader&); // Not implemented
  void operator=(const vtkPVEnSightMasterServerReader&); // Not implemented
};

#endif
