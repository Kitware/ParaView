/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMaterialLibrary.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVMaterialLibrary
 * @brief   manages visual material definitions
 *
 * vtkPVMaterialLibrary helps ParaView to load visual material definition
 * files from known, generally process relative, locations on the sever.
*/

#ifndef vtkPVMaterialLibrary_h
#define vtkPVMaterialLibrary_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

#include "vtkObject.h"
class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVMaterialLibrary : public vtkObject
{
public:
  static vtkPVMaterialLibrary* New();
  vtkTypeMacro(vtkPVMaterialLibrary, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  /**
   * Unlike vtkOSPRayMaterial::ReadFile, this searches in a number of
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

  /**
   * Returns the underlying material library.
   * When compiled withouth OSPRAY, will return nullptr
   */
  vtkObject* GetMaterialLibrary();

  /**
   * Defer to contained MaterialLibrary
   */
  bool ReadFile(const char* FileName);
  /**
   * Defer to contained MaterialLibrary
   */
  const char* WriteBuffer();
  /**
   * Defer to contained MaterialLibrary
   */
  bool ReadBuffer(const char*);

protected:
  vtkPVMaterialLibrary();
  ~vtkPVMaterialLibrary() override;

  vtkSetStringMacro(SearchPaths);

  char* SearchPaths;

  vtkObject* MaterialLibrary;

private:
  vtkPVMaterialLibrary(const vtkPVMaterialLibrary&) = delete;
  void operator=(const vtkPVMaterialLibrary&) = delete;
};

#endif
