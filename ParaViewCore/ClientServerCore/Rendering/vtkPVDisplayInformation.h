/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDisplayInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDisplayInformation - provides information about the rendering
// display and OpenGL context.
// .SECTION Description

#ifndef vtkPVDisplayInformation_h
#define vtkPVDisplayInformation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVInformation.h"
class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVDisplayInformation : public vtkPVInformation
{
public:
  static vtkPVDisplayInformation* New();
  vtkTypeMacro(vtkPVDisplayInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns if the display can be opened up on the current processes.
  static bool CanOpenDisplayLocally();

  // Description:
  // Returns true if OpenGL context supports core features required for
  // rendering.
  static bool SupportsOpenGLLocally();

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object. Calls AddInformation(info, 0).
  virtual void AddInformation(vtkPVInformation* info);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // CanOpenDisplay is set to 1 if a window can be opened on
  // the display.
  vtkGetMacro(CanOpenDisplay, int);

  // Description:
  // SupportsOpenGL is set to 1 if the OpenGL context available supports core
  // features needed for rendering.
  vtkGetMacro(SupportsOpenGL, int);

protected:
  vtkPVDisplayInformation();
  ~vtkPVDisplayInformation();

  int CanOpenDisplay;
  int SupportsOpenGL;

private:
  vtkPVDisplayInformation(const vtkPVDisplayInformation&); // Not implemented
  void operator=(const vtkPVDisplayInformation&); // Not implemented
};

#endif
