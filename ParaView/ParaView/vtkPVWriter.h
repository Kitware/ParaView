/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWriter.h
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
// .NAME vtkPVWriter - 
// .SECTION Description
// vtkPVWriter

#ifndef __vtkPVWriter_h
#define __vtkPVWriter_h

#include "vtkKWObject.h"

class vtkDataSet;
class vtkPVApplication;

class VTK_EXPORT vtkPVWriter : public vtkKWObject
{
public:
  static vtkPVWriter* New();
  vtkTypeMacro(vtkPVWriter,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);  
  
  // Description:
  // Get/Set the name of the vtk data type that this writer can write.
  vtkSetStringMacro(InputClassName);
  vtkGetStringMacro(InputClassName);
  
  // Description:
  // Get/Set the name of the actual class that implements the writer.
  vtkSetStringMacro(WriterClassName);
  vtkGetStringMacro(WriterClassName);
  
  // Description:
  // Get/Set the description of the file type supported by this
  // writer.
  vtkSetStringMacro(Description);
  vtkGetStringMacro(Description);
  
  // Description:
  // Get/Set the file extension supported by this writer.
  vtkSetStringMacro(Extension);
  vtkGetStringMacro(Extension);
  
  // Description:
  // Get/Set whether the file writer is for parallel file formats.
  vtkSetMacro(Parallel, int);
  vtkGetMacro(Parallel, int);
  vtkBooleanMacro(Parallel, int);
  
  // Description:
  // Check whether this writer supports the given VTK data set's type.
  int CanWriteData(vtkDataSet* data, int parallel);
  
  // Description:
  // This just returns the application typecast correctly.
  vtkPVApplication* GetPVApplication();
  
  // Description:
  // Write use this writer to store the given data set to the given
  // file.
  void Write(const char* fileName, const char* dataTclName, int numProcs,
             int ghostLevel);
  
protected:
  vtkPVWriter();
  ~vtkPVWriter();
  
  char* InputClassName;
  char* WriterClassName;
  char* Description;
  char* Extension;
  int Parallel;
};

#endif
