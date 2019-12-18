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
/**
 * @class   vtkPVEnSightMasterServerReader
 *
*/

#ifndef vtkPVEnSightMasterServerReader_h
#define vtkPVEnSightMasterServerReader_h

#include "vtkGenericEnSightReader.h"
#include "vtkPVVTKExtensionsIOEnSightModule.h" //needed for exports

class vtkMultiProcessController;
class vtkPVEnSightMasterServerReaderInternal;
class vtkPVEnSightMasterServerTranslator;

class VTKPVVTKEXTENSIONSIOENSIGHT_EXPORT vtkPVEnSightMasterServerReader
  : public vtkGenericEnSightReader
{
public:
  static vtkPVEnSightMasterServerReader* New();
  vtkTypeMacro(vtkPVEnSightMasterServerReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * This class uses MPI communication mechanisms to verify the
   * integrity of all case files in the master file.  The get method
   * interface must use vtkMultiProcessController instead of
   * vtkMPIController because Tcl wrapping requires the class's
   * wrapper to be defined, but it is not defined if MPI is not on.
   * In client-server mode, we may still need to create an instance of
   * this class on the client process even if MPI is not compiled in.
   */
  virtual vtkMultiProcessController* GetController();
  virtual void SetController(vtkMultiProcessController* controller);
  //@}

  /**
   * Return whether we can read the file given.
   */
  int CanReadFile(const char*) override;

  //@{
  /**
   * Get the number of pieces in the file.  Valid after
   * UpdateInformation.
   */
  vtkGetMacro(NumberOfPieces, int);
  //@}

protected:
  vtkPVEnSightMasterServerReader();
  ~vtkPVEnSightMasterServerReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int ParseMasterServerFile();

  void SuperclassExecuteInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  void SuperclassExecuteData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

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
  vtkPVEnSightMasterServerReader(const vtkPVEnSightMasterServerReader&) = delete;
  void operator=(const vtkPVEnSightMasterServerReader&) = delete;
};

#endif
