/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightReaderModule.h
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
// .NAME vtkPVEnSightReaderModule - A class to handle the UI for EnSight readers
// .SECTION Description
// This is a special reader module for the EnSight readers. Since EnSight
// readers have usually multiple output, this module creates dummy
// "connection" modules for each output. These modules act as terminals
// to which users can attach other modules. The module which represent
// the properties of the reader does not have a display page and does
// not add any actors. On the other hand, the "connection" modules have
// no properties pages. Deleting the reader module (only possible if
// the connection points have no consumers) deletes the whole assembly.
//
// .SECTION See also
// vtkPVReaderModule vtkPVAdvancedReaderModule vtkGenericEnsightReader
// vtkEnSightReader vtkEnSightGoldReader vtkEnSightGoldBinaryReader
// vtkEnSight6Reader vtkEnSight6BinaryReader vtkEnSightMasterServerReader
// vtkPVEnSightVerifier

#ifndef __vtkPVEnSightReaderModule_h
#define __vtkPVEnSightReaderModule_h

#include "vtkPVReaderModule.h"

class vtkGenericEnSightReader;
class vtkEnSightReader;
class vtkPVEnSightVerifier;

class VTK_EXPORT vtkPVEnSightReaderModule : public vtkPVReaderModule
{
public:
  static vtkPVEnSightReaderModule* New();
  vtkTypeMacro(vtkPVEnSightReaderModule, vtkPVReaderModule);
    
  // Description:
  // Tries to read a given file. Return VTK_OK on success, VTK_ERROR
  // on failure. A new instance of a reader module (which contains the 
  // actual VTK reader to be used) is returned. This should be called
  // only on a prototype. 
  // The EnSight readers are special. They can have as many outputs
  // as there are parts in the file. Furthermore, these outputs can
  // be of different type. Furthermore, they recognize time and they
  // can load different data/files for different time. Furthermore,
  // the master server (or sos, or server of servers) files contain
  // references to other EnSight files. ReadFile supports all of these
  // and provides sanity checking and error handling. For master server
  // files, a vtkPVEnSightVerifier is used to make sure that the 
  // EnSight files on all processes are compatible. (Note that the
  // master server files are only supported when ParaView is compiled
  // with MPI support)
  virtual int ReadFile(const char* fname, vtkPVReaderModule*& prm);
  virtual int ReadFile(const char* fname, float timeValue,
                       vtkPVApplication* pvApp, vtkPVWindow* window);

  // Description:
  // The EnSight reader can actually be used to check a file to see if it
  // is a valid EnSight file. A vtkGenericEnSightReader is used to
  // determine if the given file is an EnSight file.  The extension is not
  // relevant.
  virtual int CanReadFile(const char* fname);

  // Description:
  // Access from trace file to specifying which point/cell variables to read
  void AddPointVariable(const char* variableName);
  void AddCellVariable(const char* variableName);
  
protected:
  vtkPVEnSightReaderModule();
  ~vtkPVEnSightReaderModule();

  char* VerifierTclName;
  vtkSetStringMacro(VerifierTclName);

  vtkPVEnSightVerifier* Verifier;

  // Given a filename, creates a (hopefully unique) tclname.
  char* CreateTclName(const char* fname);
  
  // Prompt the user for the time step loaded first.
  int InitialTimeSelection(const char* tclName,
                           vtkGenericEnSightReader* reader, float& time);

  // Prompt the user for the variables loaded first.
  int InitialVariableSelection(const char* tclName,
                               vtkGenericEnSightReader* reader);
  
  // Make sure that all case files in the master file are compatible.
  vtkEnSightReader* VerifyMasterFile(vtkPVApplication* app,
                                     const char* tclName,
                                     const char* filename);
  
  // Make sure that all time steps are compatible.
  void VerifyTimeSets(vtkPVApplication* app, const char* filename);
  // Make sure that all parts are compatible.
  int  VerifyParts(vtkPVApplication* app, const char* filename);

  void DeleteVerifier();
  void DisplayErrorMessage(const char* message, int warning);

  int NumberOfRequestedPointVariables;
  int NumberOfRequestedCellVariables;
  char** RequestedPointVariables;
  char** RequestedCellVariables;
  
private:
  vtkPVEnSightReaderModule(const vtkPVEnSightReaderModule&); // Not implemented
  void operator=(const vtkPVEnSightReaderModule&); // Not implemented
};

#endif
