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
/**
 * @class   vtkSMWriterFactory
 * @brief   is a factory or creating a writer based on the
 * data type information from the output port.
 *
 * vtkSMWriterFactory is a factory for creating a writer to write the data
 * provided at an output port. This is done whenever a new proxy definition
 * is added in the writers group.
 *
 * Alternatively, one can register prototypes using \c RegisterPrototype API.
 * The proxy definitions for the writer proxies must provide hints that
 * indicate the file extension and description for the writer.
 *
 * Once the factory has been configured, the API to create writers, get
 * available writers etc. can be used.
*/

#ifndef vtkSMWriterFactory_h
#define vtkSMWriterFactory_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMSession;
class vtkSMSessionProxyManager;
class vtkSMSourceProxy;
class vtkStringList;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMWriterFactory : public vtkSMObject
{
public:
  static vtkSMWriterFactory* New();
  vtkTypeMacro(vtkSMWriterFactory, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Cleanup all registered prototypes.
   */
  void Initialize();

  /**
   * Register a prototype.
   */
  void RegisterPrototype(const char* xmlgroup, const char* xmlname);

  /**
   * Returns true if the data from the output port can be written at all.
   */
  bool CanWrite(vtkSMSourceProxy*, unsigned int outputport);

  /**
   * Create a new writer proxy to write the data from the specified output port
   * to the file specified, if possible.
   * As internally UpdatePipeline() will be called on the source proxy,
   * in order to prevent a double pipeline execution when you want to write a
   * given timestep, you should call updatePipeline( time ) before the
   * CreateWriter call.
   * The proxybyname flag tells the factory to use the proxy name rather than
   * a filename.extension to lookup the right proxy.
   */
  vtkSMProxy* CreateWriter(
    const char* filename, vtkSMSourceProxy*, unsigned int outputport, bool proxyname = false);
  vtkSMProxy* CreateWriter(const char* filename, vtkSMSourceProxy* pxy)
  {
    return this->CreateWriter(filename, pxy, 0);
  }

  /**
   * Returns a formatted string with all supported file types.
   * An example returned string would look like:
   * \verbatim
   * "PVD Files (*.pvd);;VTK Files (*.vtk)"
   * \endverbatim
   */
  const char* GetSupportedFileTypes(vtkSMSourceProxy* source, unsigned int outputport);
  const char* GetSupportedFileTypes(vtkSMSourceProxy* source)
  {
    return this->GetSupportedFileTypes(source, 0);
  }
  /**
   * A variation on GetSupportedFileTypes that obtains a list of prototype proxy names.
  */
  const char* GetSupportedWriterProxies(vtkSMSourceProxy* source, unsigned int outputport);

  // Returns the number of registered prototypes.
  unsigned int GetNumberOfRegisteredPrototypes();

  /**
   * Every time a new proxy definition is added we check to see if it is
   * a writer and then we add it to the list of available writers.
   */
  void UpdateAvailableWriters();

  //@{
  /**
   * Add/remove a group name to look for writers in. By default
   * "writers" is included.
   */
  void AddGroup(const char* groupName);
  void RemoveGroup(const char* groupName);
  void GetGroups(vtkStringList* groups);
  //@}

  /**
   * This function is for ParaView based applications that only wish to expose
   * a subset of the writers.  If this function is never called, the reader
   * factory will expose all the writers as it has in the past.  However, if
   * any writers are specified by passing their group name and reader name to
   * this function, then only those writers will be available in any reader
   * factories created by the application.  This is intended to be called at
   * the beginning of the application's execution before any sessions are
   * created.
   */
  static void AddWriterToWhitelist(const char* writerxmlgroup, const char* writerxmlname);

protected:
  vtkSMWriterFactory();
  ~vtkSMWriterFactory() override;

private:
  vtkSMWriterFactory(const vtkSMWriterFactory&) = delete;
  void operator=(const vtkSMWriterFactory&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
