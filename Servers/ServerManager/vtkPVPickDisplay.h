/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPickDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPickDisplay - Collect the pick data.
// .SECTION Description
// This class takes an input and collects the data for display in the UI.
// It is responsible for displaying the labels on the points.

#ifndef __vtkPVPickDisplay_h
#define __vtkPVPickDisplay_h


#include "vtkPVDisplay.h"
#include "vtkClientServerID.h" // needed for ID.

class vtkDataSet;
class vtkPVProcessModule;
class vtkPVDataInformation;
class vtkProp;
class vtkProperty;
class vtkSMPart;
class vtkUnstructuredGrid;


class VTK_EXPORT vtkPVPickDisplay : public vtkPVDisplay
{
public:
  static vtkPVPickDisplay* New();
  vtkTypeRevisionMacro(vtkPVPickDisplay, vtkPVDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Connects the parts data to the plot actor.
  // All point data arrays are ploted for now.
  // Data must already be updated.
  virtual void SetInput(vtkSMPart* input);

  // Description:
  // Turns visibility on or off.
  virtual void SetVisibility(int v);
  vtkGetMacro(Visibility,int);

  // Description:
  // This also creates the vtk objects for the composite.
  // (actor, mapper, ...)
  virtual void SetProcessModule(vtkPVProcessModule *pm);
  vtkGetObjectMacro(ProcessModule,vtkPVProcessModule);

  // Description:
  // This method updates the piece that has been assigned to this process.
  virtual void Update();

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);  

  // Description:
  // Not referenced counted.  I might get rid of this reference later.
  // fixme: merge this with set input. (wait until berks proxy code.)
  virtual void SetPart(vtkSMPart* part) {this->Part = part;}
  vtkSMPart* GetPart() {return this->Part;}

  // Description:
  // PVSource calls this when it gets modified.
  void InvalidateGeometry();

  //BTX
  // Description:
  // The Probe needs access to this to fill in the UI point values.
  vtkUnstructuredGrid *GetCollectedData();
  //ETX

  // Description:
  // This should be called with visibility of PVSource.
  void SetPointLabelVisibility(int val);

protected:
  vtkPVPickDisplay();
  ~vtkPVPickDisplay();
  
  virtual void RemoveAllCaches();

  // I might get rid of this reference.
  vtkSMPart* Part;
  vtkPVProcessModule *ProcessModule;

  int GeometryIsValid;
  int Visibility;

  vtkClientServerID UpdateSuppressorID;
  vtkClientServerID DuplicateID;
  vtkClientServerID PointLabelMapperID;
  vtkClientServerID PointLabelActorID;

  // This method gets called by SetProcessModule.
  virtual void CreateParallelTclObjects(vtkPVProcessModule *pm);

  vtkPVPickDisplay(const vtkPVPickDisplay&); // Not implemented
  void operator=(const vtkPVPickDisplay&); // Not implemented
};

#endif
