/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOSPRayMaterialLibrary.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVOSPRayMaterialLibrary
 * @brief   Used to load OSPRay Material Library definitions.
 *
 * vtkPVOSPRayMaterialLibrary helps ParaView to load OSPRay Material files
 * from known, generally process relative, locations on the sever.
*/

#ifndef vtkPVOSPRayMaterialLibrary_h
#define vtkPVOSPRayMaterialLibrary_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVConfig.h"

#ifdef PARAVIEW_USE_OSPRAY
#include "vtkOSPRayMaterialLibrary.h"
class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVOSPRayMaterialLibrary
  : public vtkOSPRayMaterialLibrary
{
public:
  static vtkPVOSPRayMaterialLibrary* New();
  vtkTypeMacro(vtkPVOSPRayMaterialLibrary, vtkOSPRayMaterialLibrary);
#else

#include "vtkObject.h"
class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVOSPRayMaterialLibrary : public vtkObject
{
public:
  static vtkPVOSPRayMaterialLibrary* New();
  vtkTypeMacro(vtkPVOSPRayMaterialLibrary, vtkObject);

  // fake API for OSPRayMaterialLibrary
  bool ReadFile(const char*) { return false; };
  const char* WriteBuffer() { return nullptr; };
  bool ReadBuffer(const char*) { return false; };
#endif

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  /**
   * Unlike parent class's ReadFile, this searches in a number of
   * relative and environmental paths specified by the SearchPaths
   * member variable.
   */
  void ReadRelativeFile(const char* FileName);

  /**
   * Get a string of standard search paths (path1;path2;path3)
   * search paths are based on PV_PLUGIN_PATH,
   * plugin dir relative to executable.
   */
  vtkGetStringMacro(SearchPaths);

protected:
  vtkPVOSPRayMaterialLibrary();
  ~vtkPVOSPRayMaterialLibrary() override;

  vtkSetStringMacro(SearchPaths);

  char* SearchPaths;

private:
  vtkPVOSPRayMaterialLibrary(const vtkPVOSPRayMaterialLibrary&) = delete;
  void operator=(const vtkPVOSPRayMaterialLibrary&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkPVOSPRayMaterialLibrary.h
