/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCollectionWriter.h
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
// .NAME vtkPVCollectionWriter - Wraps a VTK file writer.
// .SECTION Description
// vtkPVCollectionWriter provides functionality for writers similar to that
// provided by vtkPVReaderModule for readers.  An instance of this
// class is configured by an XML ModuleInterface specification and
// knows how to create and use a single VTK file writer object.

#ifndef __vtkPVCollectionWriter_h
#define __vtkPVCollectionWriter_h

#include "vtkPVWriter.h"

class VTK_EXPORT vtkPVCollectionWriter : public vtkPVWriter
{
public:
  static vtkPVCollectionWriter* New();
  vtkTypeRevisionMacro(vtkPVCollectionWriter,vtkPVWriter);
  void PrintSelf(ostream& os, vtkIndent indent);  
  
  // Description:
  // Check whether this writer supports the given VTK data set's type.
  virtual int CanWriteData(vtkDataSet* data, int parallel, int numParts);
  
  // Description:
  // Write the current source's data to the collection file with the
  // given name.
  void Write(const char* fileName, vtkPVSource* pvs, int numProcs,
             int ghostLevel);
  
protected:
  vtkPVCollectionWriter();
  ~vtkPVCollectionWriter();
  
private:
  vtkPVCollectionWriter(const vtkPVCollectionWriter&); // Not implemented
  void operator=(const vtkPVCollectionWriter&); // Not implemented
};

#endif
