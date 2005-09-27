/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoxWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBoxWidgetProxy
// .SECTION Description


#ifndef __vtkSMBoxWidgetProxy_h
#define __vtkSMBoxWidgetProxy_h

#include "vtkSM3DWidgetProxy.h"

class vtkTransform;
class vtkMatrix4x4;

class VTK_EXPORT vtkSMBoxWidgetProxy : public vtkSM3DWidgetProxy
{
public:
  static vtkSMBoxWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMBoxWidgetProxy, vtkSM3DWidgetProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set methods for the iVars.
  // On UpdateVTKObjects(), these iVars are used to compute 
  // a transformation matrix which is set on the 3DWidget object
  // on the Server(and Client).
  vtkSetVector3Macro(Position,double);
  vtkGetVector3Macro(Position,double);
  vtkSetVector3Macro(Scale,double);
  vtkGetVector3Macro(Scale,double);
  vtkSetVector3Macro(Rotation,double);
  vtkGetVector3Macro(Rotation,double);
 
  // Description:
  // Called to push the values onto the VTK object.
  virtual void UpdateVTKObjects();

  // Save the proxy state in the batch file.
  virtual void SaveInBatchScript(ofstream *file);

protected:
  //BTX
  vtkSMBoxWidgetProxy();
  ~vtkSMBoxWidgetProxy();
  
  // Description:
  // Set/Get the transformation matrix.
  // Position/Rotation/Scale are not directly used by the 3DWidget.
  // Hence we compose the transformation maxtrix from these iVars.
  // These methods set/get the transformation maxtrix.
  void SetMatrix(vtkMatrix4x4* mat);
  void GetMatrix(vtkMatrix4x4* mat);
  
  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);
  virtual void CreateVTKObjects(int numObjects); 

  vtkTransform*      BoxTransform;

  //Current iVars.
  double Position[3];
  double Rotation[3];
  double Scale[3];

private:
  vtkSMBoxWidgetProxy(const vtkSMBoxWidgetProxy&); // Not implemented
  void operator=(const vtkSMBoxWidgetProxy&); // Not implemented
  //ETX
};

#endif
