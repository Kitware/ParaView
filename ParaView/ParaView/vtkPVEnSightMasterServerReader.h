/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightMasterServerReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVEnSightMasterServerReader - 
// .SECTION Description

#ifndef __vtkPVEnSightMasterServerReader_h
#define __vtkPVEnSightMasterServerReader_h

#include "vtkGenericEnSightReader.h"

class vtkMPIController;
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
  virtual void SetController(vtkMPIController* controller);
  
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
  vtkMPIController* Controller;
  
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
