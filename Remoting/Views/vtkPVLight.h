/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLight.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVLight
 *
 * vtkPVLight extends vtkLight with controls that are specific to
 * OSPRay. When OSPRay is not enabled, at compile or runtime, they do
 * nothing.
*/

#ifndef vtkPVLight_h
#define vtkPVLight_h

#include "vtkLight.h"
#include "vtkRemotingViewsModule.h" //needed for exports

#include <string> //needed for std::string

class vtkInformationStringKey;

#define VTK_LIGHT_TYPE_AMBIENT_LIGHT 4

class VTKREMOTINGVIEWS_EXPORT vtkPVLight : public vtkLight
{
public:
  static vtkPVLight* New();
  vtkTypeMacro(vtkPVLight, vtkLight);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * When not, 0.0, the light will produce soft shadows.
   */
  void SetRadius(double r);

  static vtkInformationStringKey* LIGHT_NAME();
  /**
   * Name to identify this light to the user.
   */
  void SetName(const std::string& name);
  std::string GetName();

  /**
   * Overridden for special treatment of OSPRay specific
   * ambient light type.
   */
  virtual void SetLightType(int t) override;

protected:
  vtkPVLight();
  ~vtkPVLight();

private:
  vtkPVLight(const vtkPVLight&) = delete;
  void operator=(const vtkPVLight&) = delete;
};

#endif
