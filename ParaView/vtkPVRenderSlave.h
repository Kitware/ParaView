/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRenderSlave.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkPVRenderSlave - Off screen rendering in a slave process.
// .SECTION Description
// Try rendering in the slave process and transmitting the results
// to the master UI process.

#ifndef __vtkPVRenderSlave_h
#define __vtkPVRenderSlave_h

#include "vtkKWObject.h"
#include "vtkPVSlave.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"


class VTK_EXPORT vtkPVRenderSlave : public vtkObject
{
public:
  static vtkPVRenderSlave* New();
  vtkTypeMacro(vtkPVRenderSlave,vtkObject);

  // Description:
  // This renders and transmits the image back.
  void Render();

  // Description:
  // The PVSlave is like a controller but is used by the slaves.
  // It may be the start of a communicator.
  vtkSetObjectMacro(PVSlave, vtkPVSlave);
  vtkGetObjectMacro(PVSlave, vtkPVSlave);

  // Description:
  // Access to renderer for adding actors ...
  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

protected:
  vtkPVRenderSlave();
  ~vtkPVRenderSlave();
  vtkPVRenderSlave(const vtkPVRenderSlave&) {};
  void operator=(const vtkPVRenderSlave&) {};

  vtkRenderWindow *RenderWindow;
  vtkRenderer *Renderer;
  vtkPVSlave *PVSlave;
};


#endif


