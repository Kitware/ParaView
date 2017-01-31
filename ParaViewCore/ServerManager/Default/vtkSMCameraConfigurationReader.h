/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMCameraConfigurationReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMCameraConfigurationReader
 * @brief   A reader for XML camera configuration.
 *
 *
 * A reader for XML camera configuration. Reades camera configuration files.
 * writen by the vtkSMCameraConfigurationWriter.
 *
 * @sa
 * vtkSMCameraConfigurationWriter, vtkSMProxyConfigurationReader
 *
 * @par Thanks:
 * This class was contributed by SciberQuest Inc.
*/

#ifndef vtkSMCameraConfigurationReader_h
#define vtkSMCameraConfigurationReader_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMProxyConfigurationReader.h"

class vtkSMProxy;
class vtkPVXMLElement;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMCameraConfigurationReader
  : public vtkSMProxyConfigurationReader
{
public:
  vtkTypeMacro(vtkSMCameraConfigurationReader, vtkSMProxyConfigurationReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkSMCameraConfigurationReader* New();

  /**
   * Set the render view proxy to extract camera properties from.
   */
  void SetRenderViewProxy(vtkSMProxy* rvProxy);

  //@{
  /**
   * Read the named file, and push the properties into the underying
   * managed render view proxy. This will make sure the renderview is
   * updated after the read.
   */
  virtual int ReadConfiguration(const char* filename) VTK_OVERRIDE;
  virtual int ReadConfiguration(vtkPVXMLElement* x) VTK_OVERRIDE;
  // unhide
  virtual int ReadConfiguration() VTK_OVERRIDE { return this->Superclass::ReadConfiguration(); }
  //@}

protected:
  vtkSMCameraConfigurationReader();
  virtual ~vtkSMCameraConfigurationReader();

  // Protect the superclass's SetProxy, clients are forced to use
  // SetRenderViewProxy
  void SetProxy(vtkSMProxy*) VTK_OVERRIDE { vtkErrorMacro("Use SetRenderViewProxy."); }

private:
  vtkSMCameraConfigurationReader(const vtkSMCameraConfigurationReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMCameraConfigurationReader&) VTK_DELETE_FUNCTION;
};

#endif
