/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVEnSightArraySelection.h
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
// .NAME vtkPVEnSightArraySelection - widget to select a set of data arrays.
// .SECTION Description
// vtkPVEnSightArraySelection is used for selecting which set of data arrays
// to load when from an using a vtkEnSightReader.

#ifndef __vtkPVEnSightArraySelection_h
#define __vtkPVEnSightArraySelection_h

#include "vtkPVArraySelection.h"

class VTK_EXPORT vtkPVEnSightArraySelection : public vtkPVArraySelection
{
public:
  static vtkPVEnSightArraySelection* New();
  vtkTypeMacro(vtkPVEnSightArraySelection, vtkPVArraySelection);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Method for setting the value of the VTKReader from the widget.
  // Used internally when user hits Accept.
  virtual void Accept();
  
  // Description:
  // Method for setting the value of the widget from the VTKReader.
  // Used internally when user hits Reset
  virtual void Reset();
  
  // Description:
  // Save this widget to a file.
  virtual void SaveInTclScript(ofstream *file);

protected:
  vtkPVEnSightArraySelection();
  ~vtkPVEnSightArraySelection();

private:
  vtkPVEnSightArraySelection(const vtkPVEnSightArraySelection&); // Not implemented
  void operator=(const vtkPVEnSightArraySelection&); // Not implemented
};


#endif
