/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProxyKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkPVProxyKeyFrame_h
#define __vtkPVProxyKeyFrame_h

#include "vtkPVKeyFrame.h"
class vtkSMProxy;

class VTK_EXPORT vtkPVProxyKeyFrame : public vtkPVKeyFrame
{
public:
  vtkTypeRevisionMacro(vtkPVProxyKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Method to set the key frame using a proxy.
  // Subclasses copy over selected properties from the
  // proxy over.
  virtual void SetKeyValue(vtkSMProxy*) = 0;

protected:
  vtkPVProxyKeyFrame();
  ~vtkPVProxyKeyFrame();
  
private:
  vtkPVProxyKeyFrame(const vtkPVProxyKeyFrame&); // Not implemented.
  void operator=(const vtkPVProxyKeyFrame&); // Not implemented.
};


#endif 

