/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPassThrough.h
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
// .NAME vtkPVPassThrough - A PVSource for managing multiplt outputs.
// .SECTION Description
// This class is like a filter that does nothing.  When a source has more
// than one output, one of these sources is created for each output.
// This way, ParaView can keep a one to one mapping between sources and data.


#ifndef __vtkPVPassThrough_h
#define __vtkPVPassThrough_h

#include "vtkPVSource.h"

class VTK_EXPORT vtkPVPassThrough : public vtkPVSource
{
public:
  static vtkPVPassThrough* New();
  vtkTypeRevisionMacro(vtkPVPassThrough, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // For now, we will write the passthrough filter into the tcl script.
  // this filter has special code to set the input of the passthrough filter.
  // In the future, I expect to eliminate the pass through filter from
  // the script.
  virtual void SaveInTclScript(ofstream *file, int interactiveFlag, int vtkFlag);

  // Description:
  // This value is used to connect the passthrough filter to the inputs nth output.
  // It is currently used only for saving the pipeline in a vtk tcl script.
  // This assumes "GetOutput 3" is the way to get the third output.
  vtkSetMacro(OutputNumber, int);
  vtkGetMacro(OutputNumber, int);
    
protected:
  vtkPVPassThrough();
  ~vtkPVPassThrough();
  
  int OutputNumber;

  vtkPVPassThrough(const vtkPVPassThrough&); // Not implemented
  void operator=(const vtkPVPassThrough&); // Not implemented
};

#endif
