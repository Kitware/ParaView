/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAdvancedReaderModule.h
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
