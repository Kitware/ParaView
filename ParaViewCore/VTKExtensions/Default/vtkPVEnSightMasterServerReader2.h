/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightMasterServerReader2.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVEnSightMasterServerReader2 - 
// .SECTION Description

#ifndef __vtkPVEnSightMasterServerReader2_h
#define __vtkPVEnSightMasterServerReader2_h

#include "vtkPGenericEnSightReader.h"

class vtkMultiProcessController;
class vtkPVEnSightMasterServerReader2Internal;
class vtkPVEnSightMasterServerTranslator;

class VTK_EXPORT vtkPVEnSightMasterServerReader2 : public vtkPGenericEnSightReader
{
public:
  static vtkPVEnSightMasterServerReader2* New();
  vtkTypeMacro(vtkPVEnSightMasterServerReader2, vtkPGenericEnSightReader);
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

  // Description:
  // Set the name of the file to be read.
  void SetCaseFileName(const char* fileName);

  // Description:
  // Get the number of point or cell arrays available in the input.
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();

  // Description:
  // Get the name of the point or cell array with the given index in
  // the input.
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);

  // Description:
  // Get/Set whether the point or cell array with the given name is to
  // be read.
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void SetCellArrayStatus(const char* name, int status);

  // Description:
  // Set the byte order of the file (remember, more Unix workstations
  // write big endian whereas PCs write little endian). Default is
  // big endian (since most older PLOT3D files were written by
  // workstations).
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  void SetByteOrder(int byteOrder);
  int GetByteOrder();
  const char *GetByteOrderAsString();

protected:
  vtkPVEnSightMasterServerReader2();
  ~vtkPVEnSightMasterServerReader2();
  
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  int ParseMasterServerFile();
  
  
  // The MPI controller used to communicate with the instances in
  // other processes.
  vtkMultiProcessController* Controller;
  
  // The number of pieces available in the file.
  int NumberOfPieces;
  
  // Internal implementation details.
  vtkPVEnSightMasterServerReader2Internal* Internal;
  
  // The extent translator used to provide the correct breakdown of
  // pieces across processes.
  vtkPVEnSightMasterServerTranslator* ExtentTranslator;
  
  // Whether an error occurred during ExecuteInformation.
  int InformationError;
  
private:
  vtkPVEnSightMasterServerReader2(const vtkPVEnSightMasterServerReader2&); // Not implemented
  void operator=(const vtkPVEnSightMasterServerReader2&); // Not implemented
};

#endif
