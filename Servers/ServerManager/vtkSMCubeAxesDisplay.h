/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCubeAxesDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCubeAxesDisplay - Collect the pick data.
// .SECTION Description
// This class takes an input and collects the data for display in the UI.
// It is responsible for displaying the labels on the points.

#ifndef __vtkSMCubeAxesDisplay_h
#define __vtkSMCubeAxesDisplay_h


#include "vtkSMDisplay.h"

class vtkDataSet;
class vtkPVDataInformation;
class vtkProp;
class vtkSMProxy;
class vtkProperty;
class vtkSMSourceProxy;
class vtkUnstructuredGrid;


class VTK_EXPORT vtkSMCubeAxesDisplay : public vtkSMDisplay
{
public:
  static vtkSMCubeAxesDisplay* New();
  vtkTypeRevisionMacro(vtkSMCubeAxesDisplay, vtkSMDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Not implemented yet.  Client still does this.
  virtual void AddToRenderer(vtkClientServerID rendererID);
  virtual void RemoveFromRenderer(vtkClientServerID rendererID);
//ETX

  // Description:
  // Connects the parts data to the plot actor.
  // All point data arrays are ploted for now.
  // Data must already be updated.
  virtual void SetInput(vtkSMSourceProxy* input);

  // Description:
  // Turns visibility on or off.
  virtual void SetVisibility(int v);
  vtkGetMacro(Visibility,int);

  // Description:
  // This method updates the piece that has been assigned to this process.
  virtual void Update();

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);  

  // Description:
  // PVSource calls this when it gets modified.
  void InvalidateGeometry();

protected:
  vtkSMCubeAxesDisplay();
  ~vtkSMCubeAxesDisplay();
  
  virtual void RemoveAllCaches();
  int NumberOfCaches;
  double **Caches;

  int GeometryIsValid;
  int Visibility;

  vtkSMProxy* CubeAxesProxy;
  vtkSMSourceProxy* Input;

  virtual void CreateVTKObjects(int num);

  vtkSMCubeAxesDisplay(const vtkSMCubeAxesDisplay&); // Not implemented
  void operator=(const vtkSMCubeAxesDisplay&); // Not implemented
};

#endif
