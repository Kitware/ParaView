/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMaterialLibraryProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMMaterialLibraryProxy
 * @brief   proxy for a camera.
 *
 * This a proxy for controlling vtkOSPRayMaterialLibraries on various nodes.
 * In particular we use it to ensure that all rendering processes have
 * a consistent set of materials.
*/

#ifndef vtkSMMaterialLibraryProxy_h
#define vtkSMMaterialLibraryProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMMaterialLibraryProxy : public vtkSMProxy
{
public:
  static vtkSMMaterialLibraryProxy* New();
  vtkTypeMacro(vtkSMMaterialLibraryProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Copies the Material library on the root node of server to the client.
   */
  void Synchronize();

  /**
   * Reads default materials on the process.
   */
  void LoadDefaultMaterials();

  /**
   * Reads and specified materials.
   */
  void LoadMaterials(const char*);

  /**
   * Overridden to control load from server file system.
   */
  void UpdateVTKObjects() override;

protected:
  vtkSMMaterialLibraryProxy();
  ~vtkSMMaterialLibraryProxy() override;

private:
  vtkSMMaterialLibraryProxy(const vtkSMMaterialLibraryProxy&) = delete;
  void operator=(const vtkSMMaterialLibraryProxy&) = delete;
};

#endif
