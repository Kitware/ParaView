/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWEntry.h
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
// .NAME vtkKWEntry - a single line text entry widget
// .SECTION Description
// A simple widget used for collecting keyboard input from the user. This
// widget provides support for single line input.

#ifndef __vtkKWEntry_h
#define __vtkKWEntry_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWEntry : public vtkKWWidget
{
public:
  static vtkKWEntry* New();
  vtkTypeMacro(vtkKWEntry,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the value of the entry in a few different formats.
  // In the SetValue method with float, the second argument is the
  // number of decimal places to display.
  void SetValue(const char *);
  void SetValue(int a);
  void SetValue(float f,int size);
  char *GetValue();
  int GetValueAsInt();
  float GetValueAsFloat();
  
  // Description:
  // The width is the number of charaters wide the entry box can fit.
  // To keep from changing behavior of the entry,  the default
  // value is -1 wich means the width is not explicitely set.
  void SetWidth(int width);
  vtkGetMacro(Width, int);

protected:
  vtkKWEntry();
  ~vtkKWEntry();
  
  vtkSetStringMacro(ValueString);
  vtkGetStringMacro(ValueString);
  
  char *ValueString;
  int Width;
private:
  vtkKWEntry(const vtkKWEntry&); // Not implemented
  void operator=(const vtkKWEntry&); // Not Implemented
};


#endif


