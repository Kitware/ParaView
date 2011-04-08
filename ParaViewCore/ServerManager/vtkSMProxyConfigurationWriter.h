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
// .NAME vtkSMProxyConfigurationWriter - Base readers of a vtkSMProxy's vtkSMProperty's.
//
// .SECTION Description
// vtkSMProxyConfigurationWriter writes state for properties for a single
// proxy. Internally the ParaView state machinery is employed.
//
// The notion of proxy configuration is similar to state but lighter
// as the proxy its domains and and its server side objects are assumed to
// already exist. Configuration also provides subseting mechanism so that
// properties may be excluded if needed.
//
// Subsetting is achieved through a specialized iterator derived from
// vtkSMPropertyIterator.
//
// .SECTION See also
// vtkSMProxyConfigurationReader, vtkSMPropertyIterator, vtkSMNamedPropertyIterator
//
// .SECTION Thanks
// This class was contribued by SciberQuest Inc.
#ifndef __vtkSMProxyConfigurationWriter_h
#define __vtkSMProxyConfigurationWriter_h

#include "vtkSMObject.h"

class vtkSMPropertyIterator;
class vtkSMProxy;
class vtkStringList;

class VTK_EXPORT vtkSMProxyConfigurationWriter : public vtkSMObject
{
public:
  static vtkSMProxyConfigurationWriter* New();
  vtkTypeMacro(vtkSMProxyConfigurationWriter, vtkSMObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the proxy to write out.
  virtual void SetProxy(vtkSMProxy *proxy);
  vtkGetObjectMacro(Proxy,vtkSMProxy);

  // Description:
  // Set the ieterator used to traverse properties during the write.
  // If no iterator is set then all properties are written.
  virtual void SetPropertyIterator(vtkSMPropertyIterator *iter);
  vtkGetObjectMacro(PropertyIterator,vtkSMPropertyIterator);

  // Description:
  // Set/Get the file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set/get file meta data.
  vtkSetStringMacro(FileIdentifier);
  vtkGetStringMacro(FileIdentifier);

  vtkSetStringMacro(FileDescription);
  vtkGetStringMacro(FileDescription);

  vtkSetStringMacro(FileExtension);
  vtkGetStringMacro(FileExtension);

  // Description:
  // Return the writer version string.
  virtual const char *GetWriterVersion(){ return "1.0"; }

  // Description:
  // Write the proxy's state directly to an XML file, in PV state format.
  virtual int WriteConfiguration();
  virtual int WriteConfiguration(const char *fileName);
  // Description:
  // Write the proxy's state to a stream, in PV state format.
  virtual int WriteConfiguration(ostream &os);


protected:
  vtkSMProxyConfigurationWriter();
  virtual ~vtkSMProxyConfigurationWriter();

private:
  vtkSMProxyConfigurationWriter(const vtkSMProxyConfigurationWriter&); // Not implemented
  void operator=(const vtkSMProxyConfigurationWriter&); // Not implemented

private:
  char *FileName;
  //-------------------
  vtkSMProxy *Proxy;
  vtkSMPropertyIterator *PropertyIterator;
  //-------------------
  char *FileIdentifier;
  char *FileDescription;
  char *FileExtension;
};

#endif

