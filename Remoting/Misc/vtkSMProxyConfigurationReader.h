/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMProxyConfigurationReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxyConfigurationReader
 * @brief   Base readers of a vtkSMProxy's vtkSMProperty's.
 *
 *
 * vtkSMProxyConfigurationReader reads state for properties for a single
 * proxy. Internally the ParaView state machinery is employed.
 *
 * The notion of proxy configuration is similar to state but lighter
 * as the proxy its domains and and its server side objects are assumed to
 * already exist. Configuration also provides subseting mechanism so that
 * properties may be excluded if needed.
 *
 * The subsetting mechanism is implemented in the writer, the reader simply
 * reads which ever properties are found.
 *
 * @sa
 * vtkSMProxyConfigurationWriter
 *
 * @par Thanks:
 * This class was contribued by SciberQuest Inc.
*/

#ifndef vtkSMProxyConfigurationReader_h
#define vtkSMProxyConfigurationReader_h

#include "vtkRemotingMiscModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProxy;

class VTKREMOTINGMISC_EXPORT vtkSMProxyConfigurationReader : public vtkSMObject
{
public:
  static vtkSMProxyConfigurationReader* New();
  vtkTypeMacro(vtkSMProxyConfigurationReader, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the file name.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set the proxy to write out.
   */
  virtual void SetProxy(vtkSMProxy* proxy);
  vtkGetObjectMacro(Proxy, vtkSMProxy);
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

  //@{
  /**
   * Turns on/off proxy type validation. If on then the proxy's
   * type is compared with that found in the configuration file.
   * The read fails if they do not match. The feature is on by
   * default.
   */
  vtkSetMacro(ValidateProxyType, int);
  vtkGetMacro(ValidateProxyType, int);
  //@}

  /**
   * Return the reader version.
   */
  virtual const char* GetReaderVersion() { return "1.0"; }

  /**
   * Return true if the reader can read the specified version.
   */
  virtual bool CanReadVersion(const char* version);

  //@{
  /**
   * Read the configuration from the file. UpdateVTKObjects
   * is intentionally not called so that caller may have full
   * control as to when the push from client to server takes
   * place.
   */
  virtual int ReadConfiguration();
  virtual int ReadConfiguration(const char* filename);
  //@}
  /**
   * Read the configuration from the stream. PV state machinery is
   * employed.
   */
  virtual int ReadConfiguration(vtkPVXMLElement* xmlStream);

protected:
  vtkSMProxyConfigurationReader();
  ~vtkSMProxyConfigurationReader() override;

private:
  char* FileName;
  int ValidateProxyType;
  //-------------------
  vtkSMProxy* Proxy;
  //-------------------
  char* FileIdentifier;
  char* FileDescription;
  char* FileExtension;

private:
  vtkSMProxyConfigurationReader(const vtkSMProxyConfigurationReader&) = delete;
  void operator=(const vtkSMProxyConfigurationReader&) = delete;
};

#endif
