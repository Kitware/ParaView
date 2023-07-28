// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVMaterialLibrary
 * @brief   manages visual material definitions
 *
 * vtkPVMaterialLibrary helps ParaView to load visual material definition
 * files from known, generally process relative, locations on the sever.
 *
 * This class does nothing without raytracing (module VTK_MODULE_ENABLE_VTK_RenderingRayTracing
 * disabled)
 */

#ifndef vtkPVMaterialLibrary_h
#define vtkPVMaterialLibrary_h

#include "vtkRemotingViewsModule.h" //needed for exports

#include "vtkObject.h"
#include <string> //for std::string

class VTKREMOTINGVIEWS_EXPORT vtkPVMaterialLibrary : public vtkObject
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
  void WriteFile(const std::string& filename);
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
