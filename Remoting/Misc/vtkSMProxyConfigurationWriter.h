/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyConfigurationWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxyConfigurationWriter
 * @brief   Base readers of a vtkSMProxy's vtkSMProperty's.
 *
 *
 * vtkSMProxyConfigurationWriter writes state for properties for a single
 * proxy. Internally the ParaView state machinery is employed.
 *
 * The notion of proxy configuration is similar to state but lighter
 * as the proxy its domains and and its server side objects are assumed to
 * already exist. Configuration also provides subseting mechanism so that
 * properties may be excluded if needed.
 *
 * Subsetting is achieved through a specialized iterator derived from
 * vtkSMPropertyIterator.
 *
 * @sa
 * vtkSMProxyConfigurationReader, vtkSMPropertyIterator, vtkSMNamedPropertyIterator
 *
 * @par Thanks:
 * This class was contribued by SciberQuest Inc.
*/

#ifndef vtkSMProxyConfigurationWriter_h
#define vtkSMProxyConfigurationWriter_h

#include "vtkRemotingMiscModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMPropertyIterator;
class vtkSMProxy;
class vtkStringList;

class VTKREMOTINGMISC_EXPORT vtkSMProxyConfigurationWriter : public vtkSMObject
{
public:
  static vtkSMProxyConfigurationWriter* New();
  vtkTypeMacro(vtkSMProxyConfigurationWriter, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the proxy to write out.
   */
  virtual void SetProxy(vtkSMProxy* proxy);
  vtkGetObjectMacro(Proxy, vtkSMProxy);
  //@}

  //@{
  /**
   * Set the ieterator used to traverse properties during the write.
   * If no iterator is set then all properties are written.
   */
  virtual void SetPropertyIterator(vtkSMPropertyIterator* iter);
  vtkGetObjectMacro(PropertyIterator, vtkSMPropertyIterator);
  //@}

  //@{
  /**
   * Set/Get the file name.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set/get file meta data.
   */
  vtkSetStringMacro(FileIdentifier);
  vtkGetStringMacro(FileIdentifier);
  //@}

  vtkSetStringMacro(FileDescription);
  vtkGetStringMacro(FileDescription);

  vtkSetStringMacro(FileExtension);
  vtkGetStringMacro(FileExtension);

  /**
   * Return the writer version string.
   */
  virtual const char* GetWriterVersion() { return "1.0"; }

  //@{
  /**
   * Write the proxy's state directly to an XML file, in PV state format.
   */
  virtual int WriteConfiguration();
  virtual int WriteConfiguration(const char* fileName);
  //@}
  /**
   * Write the proxy's state to a stream, in PV state format.
   */
  virtual int WriteConfiguration(ostream& os);

protected:
  vtkSMProxyConfigurationWriter();
  ~vtkSMProxyConfigurationWriter() override;

private:
  vtkSMProxyConfigurationWriter(const vtkSMProxyConfigurationWriter&) = delete;
  void operator=(const vtkSMProxyConfigurationWriter&) = delete;

private:
  char* FileName;
  //-------------------
  vtkSMProxy* Proxy;
  vtkSMPropertyIterator* PropertyIterator;
  //-------------------
  char* FileIdentifier;
  char* FileDescription;
  char* FileExtension;
};

#endif
