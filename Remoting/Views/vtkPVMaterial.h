/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMaterial.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVMaterial
 * @brief   manages material definitions
 *
 * vtkPVMaterial helps ParaView to keep track of material instances.
 *
 * This class does nothing without raytracing (module VTK_MODULE_ENABLE_VTK_RenderingRayTracing
 * disabled).
 */

#ifndef vtkPVMaterial_h
#define vtkPVMaterial_h

#include "vtkRemotingViewsModule.h" //needed for exports

#include "vtkObject.h"
#include "vtkSetGet.h"

#include <initializer_list>
#include <string>

class VTKREMOTINGVIEWS_EXPORT vtkPVMaterial : public vtkObject
{
public:
  static vtkPVMaterial* New();
  vtkTypeMacro(vtkPVMaterial, vtkObject);

  //@{

  /**
   * The name of this material.
   */
  vtkSetMacro(Name, std::string);
  vtkGetMacro(Name, std::string);
  //@}
  //@{
  /**
   * The type of this material.
   */
  vtkSetMacro(Type, std::string);
  vtkGetMacro(Type, std::string);
  //@}

  /**
   * Add a variable to the OSPRayMaterialLibrary for this material.
   * \p paramName is the name of the parameter and \p value is a string containing
   * its values.
   */
  void AddVariable(const char* paramName, const char* value);

  /**
   * Removes all the shader variables and textures for this material in the OSPRayMaterialLibrary
   */
  void RemoveAllVariables();

  /**
   * Set the vtkOSPRayMaterialLibrary used to add and remove variables
   */
  void SetLibrary(vtkObject* lib);

protected:
  vtkPVMaterial() = default;
  ~vtkPVMaterial() override = default;

  std::string Name;
  std::string Type;
  vtkObject* Library = nullptr;

private:
  vtkPVMaterial(const vtkPVMaterial&) = delete;
  void operator=(const vtkPVMaterial&) = delete;
};

#endif
