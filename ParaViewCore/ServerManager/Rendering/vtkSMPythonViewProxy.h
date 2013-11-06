/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPythonViewProxy - Superclass for all view proxies
// .SECTION Description
// vtkSMPythonViewProxy is a view proxy for the vtkPythonView.

#ifndef __vtkSMPythonViewProxy_h
#define __vtkSMPythonViewProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMViewProxy.h"

class vtkImageData;
class vtkRenderer;
class vtkSMProxy;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMPythonViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMPythonViewProxy* New();
  vtkTypeMacro(vtkSMPythonViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the client-side renderer.
  vtkRenderer* GetRenderer();

  // Description:
  // Returns the client-side render window.
  vtkRenderWindow* GetRenderWindow();

  // Description:
  // Returns true if the most recent render indeed employed low-res rendering.
  virtual bool LastRenderWasInteractive();

//BTX
protected:
  vtkSMPythonViewProxy();
  ~vtkSMPythonViewProxy();

  // Description:
  // Subclasses should override this method to do the actual image capture.
  virtual vtkImageData* CaptureWindowInternal(int magnification);

private:
  vtkSMPythonViewProxy(const vtkSMPythonViewProxy&); // Not implemented
  void operator=(const vtkSMPythonViewProxy&); // Not implemented
//ETX

};

#endif // __vtkSMPythonViewProxy_h
