/*=========================================================================

  Program:   ParaView
  Module:    vtkSMWriterFactory.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMWriterFactory - is a factory or creating a writer based on the
// data type information from the output port.
// .SECTION Description
// vtkSMWriterFactory is a factory for creating a writer to write the data
// provided at an output port. The writer factory needs to be configured to
// register the writer prototypes supported by the application. This can be done
// using an XML with the following format:
// \verbatim
// <ParaViewWriters>
//    <Proxy name="[xmlname for the writer proxy]"
//           group="[optional: xmlgroup for the writer proxy, 'writers' by default]"
//           />
//    ...
// </ParaViewWriters>
// \endverbatim
//
// Alternatively, one can register prototypes using \c RegisterPrototype API.
// The proxy definitions for the writer proxies must provide hints that
// indicate the file extension and description for the writer.
//
// Once the factory has been configured, the API to create writers, get
// available writers etc. can be used.

#ifndef __vtkSMWriterFactory_h
#define __vtkSMWriterFactory_h

#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMWriterFactory : public vtkSMObject
{
public:
  static vtkSMWriterFactory* New();
  vtkTypeMacro(vtkSMWriterFactory, vtkSMObject);
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
  void RegisterPrototypes(const char* xmlgroup);

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
  // Retruns true if the data from the output port can be written at all.
  bool CanWrite(vtkSMSourceProxy*, unsigned int outputport);

  // Description:
  // Create a new writer proxy to write the data from the specified output port
  // to the file specified, if possible.
  vtkSMProxy* CreateWriter(const char* filename, vtkSMSourceProxy*,
    unsigned int outputport);
  vtkSMProxy* CreateWriter(const char* filename, vtkSMSourceProxy* pxy)
    { return this->CreateWriter(filename, pxy, 0); }

  // Description:
  // Returns a formatted string with all supported file types.
  // An example returned string would look like:
  // \verbatim
  // "PVD Files (*.pvd);;VTK Files (*.vtk)"
  // \endverbatim
  const char* GetSupportedFileTypes(vtkSMSourceProxy* source,
    unsigned int outputport);
  const char* GetSupportedFileTypes(vtkSMSourceProxy* source)
    { return this->GetSupportedFileTypes(source, 0); }

  // Description:
  // Get/Set the proxy manager (not reference counted).
  void SetProxyManager(vtkSMProxyManager* pxm)
    { this->ProxyManager = pxm; }
  vtkSMProxyManager* GetProxyManager()
    { return this->ProxyManager; }

  // Returns the number of registered prototypes.
  unsigned int GetNumberOfRegisteredPrototypes();
//BTX
protected:
  vtkSMWriterFactory();
  ~vtkSMWriterFactory();

  // To support legacy configuration files.
  void RegisterPrototype(
    const char* xmlgroup, const char* xmlname,
    const char* extensions,
    const char* description);

  vtkSMProxyManager* ProxyManager;
private:
  vtkSMWriterFactory(const vtkSMWriterFactory&); // Not implemented
  void operator=(const vtkSMWriterFactory&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif

