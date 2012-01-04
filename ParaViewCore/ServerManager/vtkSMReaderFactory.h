/*=========================================================================

  Program:   ParaView
  Module:    vtkSMReaderFactory.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMReaderFactory - is a factory for creating a reader
// proxy based on the filename/extension.
// .SECTION Description
// vtkSMReaderFactory is a factory for creating a reader that reads a particular
// file. The reader factory needs to be configured to register the reader
// prototypes supported by the application. This can be done using an XML with
// the following format:
// \verbatim
// <ParaViewReaders>
//    <Proxy name="[xmlname for the reader proxy]"
//            group="[optional: xmlgroup for the reader proxy, sources by default]"
//            />
//    ...
// </ParaViewReaders>
// \endverbatim
// Alternatively, one can register prototypes using \c RegisterPrototype API.
//
// Once the factory has been configured, the API to create readers, get
// available readers etc. can be used.

#ifndef __vtkSMReaderFactory_h
#define __vtkSMReaderFactory_h

#include "vtkSMObject.h"

class vtkStringList;
class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMSession;

class VTK_EXPORT vtkSMReaderFactory : public vtkSMObject
{
public:
  static vtkSMReaderFactory* New();
  vtkTypeMacro(vtkSMReaderFactory, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Cleanup all registered prototypes.
  void Initialize();

  // Description:
  // Register a prototype.
  void RegisterPrototype(const char* xmlgroup, const char* xmlname);
  void UnRegisterPrototype(const char* xmlgroup, const char* xmlname);

  // Description:
  // Registers all prototypes from a particular group that have the
  // "ReaderFactory" hint.
  void RegisterPrototypes(vtkSMSession* session, const char* xmlgroup);

  // Description:
  // Load configuration XML. This adds the prototypes specified in the
  // configuration XML to those already present in the factory. Use Initialize()
  // is start with an empty factory before calling this method if needed. If two
  // readers support reading the same file, the reader added more recently is
  // given priority.
  bool LoadConfigurationFile(const char* filename);
  bool LoadConfiguration(const char* xmlcontents);
  bool LoadConfiguration(vtkPVXMLElement* root);

  // Description:
  // Returns true if a reader can be determined that can read the file.
  // When this returns true, one can use GetReaderGroup() and GetReaderName() to
  // obtain the xmlgroup and xmlname for the reader that can read the file.
  // When this returns false, use GetPossibleReaders() to get the list of
  // readers that can possibly read the file.
  bool CanReadFile(const char* filename, vtkSMSession* session);

  // Description:
  // Returns the xml-name for the reader that can read the file queried by the
  // most recent CanReadFile() call. This is valid only if CanReadFile()
  // returned true.
  vtkGetStringMacro(ReaderName);

  // Description:
  // Returns the xml-group for the reader that can read the file queried by the
  // most recent CanReadFile() call. This is valid only if CanReadFile()
  // returned true.
  vtkGetStringMacro(ReaderGroup);

  // Description:
  // Get the list of readers that can possibly read the file. This is used when
  // the factory cannot determine which reader to use for reading the file. The
  // user can then choose from the provided options.
  // Returns a list of 3-tuples where the 1st string is the group, the 2nd
  // string is the reader name and the 3rd string is the reader description
  // Note that the extension test is skipped in this case.
  vtkStringList* GetPossibleReaders(const char* filename, vtkSMSession* session);

  // Description:
  // Returns a list of 3-tuples where the 1st string is the group, the 2nd
  // string is the reader name and the 3rd string is the reader description
  // This returns all the possible readers with a given connection id.
  vtkStringList* GetReaders(vtkSMSession* session);

  // Description:
  // Returls list of readers that can read the file using its extension and
  // connection id.
  // Returns a list of 3-tuples where the 1st string is the group, the 2nd
  // string is the reader name and the 3rd string is the reader description
  vtkStringList* GetReaders(const char* filename, vtkSMSession* session);

  // Description:
  // Helper method to test is a file is readable on the server side. This has
  // nothing to do with the whether the file is readable by a reader, just test
  // the file permissions etc. Internally uses the ServerFileListing proxy.
  static bool TestFileReadability(const char* filename, vtkSMSession* session);

  // Description:
  // Returns a formatted string with all supported file types.
  // \c cid is not used currently.
  // An example returned string would look like:
  // \verbatim
  // "Supported Files (*.vtk *.pvd);;PVD Files (*.pvd);;VTK Files (*.vtk)"
  // \endverbatim
  const char* GetSupportedFileTypes(vtkSMSession* session);

  // Description:
  // Helper method to check if the reader can read the given file. This is a
  // generic method that simply tries to call CanReadFile() on the reader. If
  // the reader des not support CanReadFile() then we assume the reader can read
  // the file, and return true.
  static bool CanReadFile(const char* filename, vtkSMProxy* reader);
  static bool CanReadFile(const char* filename, const char* readerxmlgroup,
    const char* readerxmlname, vtkSMSession* session);

  // Description:
  // Returns the number of registered prototypes.
  unsigned int GetNumberOfRegisteredPrototypes();

//BTX
protected:
  vtkSMReaderFactory();
  ~vtkSMReaderFactory();

  // To support legacy configuration files.
  void RegisterPrototype(
    const char* xmlgroup, const char* xmlname,
    const char* extensions,
    const char* description);

  vtkSetStringMacro(ReaderName);
  vtkSetStringMacro(ReaderGroup);

  char* ReaderName;
  char* ReaderGroup;
  vtkStringList* Readers;

private:
  vtkSMReaderFactory(const vtkSMReaderFactory&); // Not implemented
  void operator=(const vtkSMReaderFactory&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif

