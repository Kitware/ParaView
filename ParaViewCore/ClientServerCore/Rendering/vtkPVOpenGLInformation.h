/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOpenGLInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVOpenGLInformation
 * @brief   Gets OpenGL information.
 *
 * Get details of OpenGL from the render server.
*/

#ifndef vtkPVOpenGLInformation_h
#define vtkPVOpenGLInformation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVInformation.h"

#include <string> // for string type

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVOpenGLInformation : public vtkPVInformation
{
public:
  static vtkPVOpenGLInformation* New();
  vtkTypeMacro(vtkPVOpenGLInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject*);

  /**
   * Merge another information object.
   */
  virtual void AddInformation(vtkPVInformation*);

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  //@}

  //@{
  /**
   * Serialize/Deserialize the parameters that control how/what information is
   * gathered. This are different from the ivars that constitute the gathered
   * information itself. For example, PortNumber on vtkPVDataInformation
   * controls what output port the data-information is gathered from.
   */
  virtual void CopyParametersToStream(vtkMultiProcessStream&){};
  virtual void CopyParametersFromStream(vtkMultiProcessStream&){};
  //@}

  const std::string& GetVendor();
  const std::string& GetVersion();
  const std::string& GetRenderer();

  bool GetLocalDisplay();

protected:
  vtkPVOpenGLInformation();
  ~vtkPVOpenGLInformation();

  void SetVendor();

  void SetVersion();

  void SetRenderer();

  void SetLocalDisplay(bool);

private:
  vtkPVOpenGLInformation(const vtkPVOpenGLInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVOpenGLInformation&) VTK_DELETE_FUNCTION;

  bool LocalDisplay;

  std::string Vendor;
  std::string Version;
  std::string Renderer;
};

#endif
