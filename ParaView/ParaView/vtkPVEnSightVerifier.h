/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVEnSightVerifier.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVEnSightVerifier - class to verify the integrity of EnSight master files
// .SECTION Description
// The class vtkPVEnSightVerifier is used to verify the integrity of
// EnSight master ( sos - server of servers ) files. First, each process 
// reads the appropriate case file listed in the master file. Then
// information from these files are compared and any mismatches are
// reported. MPI communication mechanisms are used in this process


#ifndef __vtkPVEnSightVerifier_h
#define __vtkPVEnSightVerifier_h

#include "vtkObject.h"

class vtkEnSightReader;
class vtkMPIController;

class VTK_EXPORT vtkPVEnSightVerifier : public vtkObject
{
public:
  static vtkPVEnSightVerifier *New();
  vtkTypeRevisionMacro(vtkPVEnSightVerifier, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Case file name.
  vtkSetStringMacro(CaseFileName);
  vtkGetStringMacro(CaseFileName);

  // Description:
  // This class uses MPI communication mechanisms to verify the integrity
  // of all case files in the master file. Therefore, a controller must be
  // set before any of the XXXVerifyXXX() calls can be made.
  vtkGetObjectMacro(Controller, vtkMPIController);
  void SetController(vtkMPIController* controller);

  // Description:
  // The case filename for the local process  (which is obtained
  // from the controller). This is set in VerifyVersion()
  vtkGetStringMacro(PieceCaseFileName);

  // Description:
  // The EnSight file type. This is set in VerifyVersion()
  vtkGetMacro(EnSightVersion, int);

  // Description:
  // This method first creates a vtkEnSightMasterServerReader to determine
  // the case filename for the local process (which is obtained from the
  // controller. It then makes sure that all case filenames to be read are
  // of the same type (EnSight Gold ASCII, binary, EnSight 6 etc).
  // Only the beginning of the case files are read as a result of a call 
  // to this method (to determin the file type).
  // Geometry and variable files are not read.
  // Return values: OK, DETERMINE_VERSION_FAILED, VERSION_MISMATCH
  // DETERMINE_FILENAME_FAILED
  int VerifyVersion();

  // Description:
  // Calls reader->UpdateInformation() to verify that all casefiles
  // contain compatible time sets.
  // Return values: OK, TIME_VALUES_MISMATCH, NUMBER_OF_STEPS_MISMATCH,
  // NUMBER_OF_SETS_MISMATCH
  int ReadAndVerifyTimeSets(vtkEnSightReader* reader);

  // Description:
  // Calls reader->Update() to verify that all geometry files contain
  // compatible parts.
  // Return values: OK, NUMBER_OF_OUTPUTS_MISMATCH, 
  // OUTPUT_TYPE_MISMATCH
  int ReadAndVerifyParts(vtkEnSightReader* reader);


  // Description:
  // The last error encountered. If there was no error in the
  // last operation, this is set to OK.
  vtkGetMacro(LastError, int);

//BTX
  enum 
  {
    OK=0,
    DETERMINE_VERSION_FAILED=1,
    VERSION_MISMATCH=2,
    TIME_VALUES_MISMATCH=3,
    DETERMINE_FILENAME_FAILED=4,
    PART_MISMATCH=5,
    NUMBER_OF_STEPS_MISMATCH=6,
    NUMBER_OF_SETS_MISMATCH=7,
    NUMBER_OF_OUTPUTS_MISMATCH=8,
    OUTPUT_TYPE_MISMATCH=9
  };
//ETX

protected:
  vtkPVEnSightVerifier();
  ~vtkPVEnSightVerifier();

  vtkMPIController* Controller;
  
  char* CaseFileName;
  char* PieceCaseFileName;

  int EnSightVersion;
  int LastError;

  vtkSetStringMacro(PieceCaseFileName);
  vtkSetMacro(EnSightVersion, int);

  // Returns VTK_ERROR (to all processes) if status is not VTK_OK
  // on all processes, VTK_OK otherwise.
  int CheckForError(int status);
  
  // Returns VTK_ERROR (to all processes) if value is not the same
  // on all processes, VTK_OK otherwise.
  int CompareValues(int value);

  // Returns VTK_ERROR (to all processes) if the casefiles do not
  // contain identical time information, VTK_OK otherwise.
  int CompareTimeSets(vtkEnSightReader* reader);

  // Returns VTK_ERROR (to all processes) if the geometry files
  // do not contain identical part types and numbers.
  int CompareParts(vtkEnSightReader* reader);

private:
  vtkPVEnSightVerifier(const vtkPVEnSightVerifier&);  // Not implemented.
  void operator=(const vtkPVEnSightVerifier&);  // Not implemented.
};

#endif
