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
 */

#ifndef vtkPVMaterial_h
#define vtkPVMaterial_h

#include "vtkRemotingViewsModule.h" //needed for exports

#include "vtkObject.h"

#include <initializer_list>
#include <string>

class vtkTexture;

class VTKREMOTINGVIEWS_EXPORT vtkPVMaterial : public vtkObject
{
public:
  static vtkPVMaterial* New();
  vtkTypeMacro(vtkPVMaterial, vtkObject);

  void SetName(const std::string& name);
  const std::string& GetName();

  void SetType(const std::string& type);
  const std::string& GetType();

  void AddVariable(const char* paramName, const char* value);
  void RemoveAllVariables();

  void SetLibrary(vtkObject* lib);
  void SetTexture(vtkTexture* tex) { this->CurrentTexture = tex; }

protected:
  vtkPVMaterial() = default;
  ~vtkPVMaterial() override = default;

  std::string Name;
  std::string Type;
  vtkObject* Library = nullptr;
  vtkTexture* CurrentTexture = nullptr;

private:
  vtkPVMaterial(const vtkPVMaterial&) = delete;
  void operator=(const vtkPVMaterial&) = delete;
};

#endif
