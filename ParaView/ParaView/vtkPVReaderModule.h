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
// .NAME vtkPVReaderModule - Module representing a reader
// .SECTION Description
// The class vtkPVReaderModule is used to represent a reader (or a pipeline
// which contains a reader). A prototype for each file type is created at
// startup by parsing the (XML) configuration file. Later, when the user
// tries to open a file, the prototypes are consulted. The first prototype
// which respond positively to CanReadFile() is asked to read the file and
// create a new instance of a vtkPVReaderModule (or any subclass) to be
// added to the list of existing sources.
//
// .SECTION See also
// vtkPVAdvancedReaderModule vtkPVEnSightReaderModule

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
  // Set up the UI for this source. The only default element created is a
  // file entry. Of course, more can be added in the configuration file.
  virtual void CreateProperties();

  // Description:
  // Returns true (1) if the current reader can read the specified file,
  // false (0) otherwise. In the default implementation, this is done by
  // comparing the extension of the file to a list of extensions specified
  // by the configuration (XML) file -see AddExtension-.
  virtual int CanReadFile(const char* fname);

  // Description:
  // Tries to read a given file. Return VTK_OK on success, VTK_ERROR
  // on failure. A new instance of a reader module (which contains the 
  // actual VTK reader to be used) is returned. This should be called
  // only on a prototype. (This method replaces ClonePrototype defined
  // in superclass).
  virtual int ReadFile(const char* fname, vtkPVReaderModule*& newModule);

  // Description:
  // Add extension recognized by the reader. This is displayed in the
  // selection dialog and used in the default implementation of
  // CanReadFile(). 
  void AddExtension(const char*);
  
  // Description:
  // Get the number of registered file extensions.
  vtkIdType GetNumberOfExtensions();
  
  // Description:
  // Get the ith file extension.
  const char* GetExtension(vtkIdType i);

  // Description:
  // This tells vtkPVWindow whether it should call Accept() on the module
  // returned by ReadFile. In the default implementation, the ReadFile()
  // creates a clone and sets up all the filename (which is the only user
  // modifiable option). Since the user does not have to make any
  // selections before the file is loaded, vtkPVWindow calls Accept instead
  // of the user. This behaviour is changed in vtkPVAdvancedReaderModule
  // which first calls UpdateInformation() on the VTK reader, obtains some
  // preliminary information from the file and then prompts the user for
  // some stuff. In this situation, vtkPVWindow does not call Accept. This
  // is mainly used to avoid reading the whole file before asking the user
  // the initial configuration (for example, the user might want to load
  // only a subset of available attriutes)
  vtkSetMacro(AcceptAfterRead, int);
  vtkGetMacro(AcceptAfterRead, int);

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

  // Description: 
  // Creates and returns (by reference) a copy of this source. It will
  // create a new instance of the same type as the current object using
  // NewInstance() and then call ClonePrototype() on all widgets and add
  // these clones to it's widget list. The return value is VTK_OK is the
  // cloning was successful.
  int ClonePrototype(int makeCurrent, vtkPVReaderModule*& clone);

private:
  vtkPVReaderModule(const vtkPVReaderModule&); // Not implemented
  void operator=(const vtkPVReaderModule&); // Not implemented
};

#endif
