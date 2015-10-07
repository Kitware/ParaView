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
// .NAME vtkPVOpenGLInformation - Gets OpenGL information.
// .SECTION Description
// Get details of OpenGL from the render server.


#ifndef vtkPVOpenGLInformation_h
#define vtkPVOpenGLInformation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVInformation.h"

#include <string> // for string type

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVOpenGLInformation
    : public vtkPVInformation
{
public:
  static vtkPVOpenGLInformation* New();
  vtkTypeMacro(vtkPVOpenGLInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  //BTX
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // Serialize/Deserialize the parameters that control how/what information is
  // gathered. This are different from the ivars that constitute the gathered
  // information itself. For example, PortNumber on vtkPVDataInformation
  // controls what output port the data-information is gathered from.
  virtual void CopyParametersToStream(vtkMultiProcessStream&) {};
  virtual void CopyParametersFromStream(vtkMultiProcessStream&) {};
  //ETX

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
  vtkPVOpenGLInformation(const vtkPVOpenGLInformation&); // Not implemented
  void operator=(const vtkPVOpenGLInformation&); // Not implemented

bool LocalDisplay;

std::string Vendor;
std::string Version;
std::string Renderer;
};

#endif
