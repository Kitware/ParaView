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
// provided at an output port. This is done whenever a new proxy definition
// is added in the writers group.
//
// Alternatively, one can register prototypes using \c RegisterPrototype API.
// The proxy definitions for the writer proxies must provide hints that
// indicate the file extension and description for the writer.
//
// Once the factory has been configured, the API to create writers, get
// available writers etc. can be used.

#ifndef __vtkSMWriterFactory_h
#define __vtkSMWriterFactory_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMSession;
class vtkSMSessionProxyManager;
class vtkSMSourceProxy;
class vtkStringList;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMWriterFactory : public vtkSMObject
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

  // Description:
  // Retruns true if the data from the output port can be written at all.
  bool CanWrite(vtkSMSourceProxy*, unsigned int outputport);

  // Description:
  // Create a new writer proxy to write the data from the specified output port
  // to the file specified, if possible.
  // As internally UpdatePipeline() will be called on the source proxy,
  // in order to prevent a double pipeline execution when you want to write a
  // given timestep, you should call updatePipeline( time ) before the
  // CreateWriter call.
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

  // Returns the number of registered prototypes.
  unsigned int GetNumberOfRegisteredPrototypes();

  // Description:
  // Every time a new proxy definition is added we check to see if it is
  // a writer and then we add it to the list of available writers.
  void UpdateAvailableWriters();

  // Description:
  // Add/remove a group name to look for writers in. By default
  // "writers" is included.
  void AddGroup(const char* groupName);
  void RemoveGroup(const char* groupName);
  void GetGroups(vtkStringList* groups);

//BTX
protected:
  vtkSMWriterFactory();
  ~vtkSMWriterFactory();

private:
  vtkSMWriterFactory(const vtkSMWriterFactory&); // Not implemented
  void operator=(const vtkSMWriterFactory&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif

