/*=========================================================================

  Program:   ParaView
  Module:    vtkPVReaderModule.h
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
// .NAME vtkPVReaderModule - A class to handle the UI for vtkGlyph3D
// .SECTION Description


#ifndef __vtkPVReaderModule_h
#define __vtkPVReaderModule_h

#include "vtkPVSource.h"

class vtkPVFileEntry;
//BTX
template <class value>
class vtkVector;
template <class value>
class vtkVectorIterator;
//ETX

class VTK_EXPORT vtkPVReaderModule : public vtkPVSource
{
public:
  static vtkPVReaderModule* New();
  vtkTypeMacro(vtkPVReaderModule, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  virtual void CreateProperties();

  // Description:
  // Create the PVReaderModule widgets.
  virtual void InitializePrototype();

  // Description:
  virtual int CanReadFile(const char* fname);

  // Description:
  virtual int ReadFile(const char* fname, vtkPVReaderModule*& prm);

  // Description:
  void AddExtension(const char*);
  
  // Description:
  // Get the number of registered file extensions.
  vtkIdType GetNumberOfExtensions();
  
  // Description:
  // Get the ith file extension.
  const char* GetExtension(vtkIdType i);

  // Description:
  // This tells vtkPVWindow whether it should call Accept() on
  // the module returned by ReadFile.
  vtkSetMacro(AcceptAfterRead, int);
  vtkGetMacro(AcceptAfterRead, int);

  // Description: Creates and returns (by reference) a copy of this
  // source. It will create a new instance of the same type as the current
  // object using NewInstance() and then call ClonePrototype() on all
  // widgets and add these clones to it's widget list. The return
  // value is VTK_OK is the cloning was successful.
  int ClonePrototype(int makeCurrent, vtkPVReaderModule*& clone);

protected:
  vtkPVReaderModule();
  ~vtkPVReaderModule();

  const char* ExtractExtension(const char* fname);

  vtkPVFileEntry* FileEntry;
  int AcceptAfterRead;

//BTX
  vtkVector<const char*>* Extensions;
  vtkVectorIterator<const char*>* Iterator;
//ETX

private:
  vtkPVReaderModule(const vtkPVReaderModule&); // Not implemented
  void operator=(const vtkPVReaderModule&); // Not implemented
};

#endif
