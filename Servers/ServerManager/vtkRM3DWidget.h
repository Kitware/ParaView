/*=========================================================================

  Program:   ParaView
  Module:    vtkRM3DWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRM3DWidget - 
// .SECTION Description


#ifndef __vtkRM3DWidget_h
#define __vtkRM3DWidget_h

#include "vtkRMObject.h"

class vtkPVProcessModule;


class VTK_EXPORT vtkRM3DWidget : public vtkRMObject
{
public:

  vtkTypeRevisionMacro(vtkRM3DWidget, vtkRMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkPVProcessModule *pm, vtkClientServerID rendererID, 
    vtkClientServerID interactorID);
 
  vtkClientServerID GetWidget3DID () { return this->Widget3DID; }

  virtual void PlaceWidget(double bds[6]);
  virtual void SetVisibility(int visibility);

protected:
//BTX
  vtkRM3DWidget();
  ~vtkRM3DWidget();
  
  vtkPVProcessModule *PVProcessModule;
  vtkClientServerID Widget3DID; 

private:  
  vtkRM3DWidget(const vtkRM3DWidget&); // Not implemented
  void operator=(const vtkRM3DWidget&); // Not implemented
//ETX
};

#endif
