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

#ifndef __vtkPVEnSightReaderModule_h
#define __vtkPVEnSightReaderModule_h

#include "vtkPVReaderModule.h"

class vtkGenericEnSightReader;

class VTK_EXPORT vtkPVEnSightReaderModule : public vtkPVReaderModule
{
public:
  static vtkPVEnSightReaderModule* New();
  vtkTypeMacro(vtkPVEnSightReaderModule, vtkPVReaderModule);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // Create the PVEnSightReaderModule widgets.
  virtual void InitializePrototype();

  // Description:
  virtual int ReadFile(const char* fname, vtkPVReaderModule*& prm);

protected:
  vtkPVEnSightReaderModule();
  ~vtkPVEnSightReaderModule();

  char* CreateTclName(const char* fname);
  
  int InitialTimeSelection(const char* tclName, 
			   vtkGenericEnSightReader* reader, float& time);

private:
  vtkPVEnSightReaderModule(const vtkPVEnSightReaderModule&); // Not implemented
  void operator=(const vtkPVEnSightReaderModule&); // Not implemented
};

#endif
