/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODPartDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLODPartDisplay - This contains all the LOD, mapper and actor stuff.
// .SECTION Description
// This is the part displays for serial execution of paraview.
// I handles all of the decimation levels of detail.


#ifndef __vtkPVLODPartDisplay_h
#define __vtkPVLODPartDisplay_h


#include "vtkPVPartDisplay.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID
class vtkDataSet;
class vtkPVLODPartDisplayInformation;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkPVPart;
class vtkRMScalarBarWidget;

class VTK_EXPORT vtkPVLODPartDisplay : public vtkPVPartDisplay
{
public:
  static vtkPVLODPartDisplay* New();
  vtkTypeRevisionMacro(vtkPVLODPartDisplay, vtkPVPartDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of bins per axes on the quadric decimation filter.
  virtual void SetLODResolution(int res);

  // Description:
  // Toggles the mappers to use immediate mode rendering or display lists.
  virtual void SetUseImmediateMode(int val);

  // Description:
  // Change the color mode to map scalars or 
  // use unsigned char arrays directly.
  // MapScalarsOff only works when coloring by an array 
  // on unsigned chars with 1 or 3 components.
  virtual void SetDirectColorFlag(int val);
  virtual void SetScalarVisibility(int val);
  virtual void ColorByArray(vtkRMScalarBarWidget *colorMap, int field);

  // Description:
  // Option to use a 1d texture map for the attribute color.
  virtual void SetInterpolateColorsFlag(int val);

  // Description:
  // This method updates the piece that has been assigned to this process.
  // It also gathers the data information.
  virtual void Update();

  // Description:
  // For flip books.
  virtual void RemoveAllCaches();
  virtual void CacheUpdate(int idx, int total);
            
  //=============================================================== 
  // Description:
  // These access methods are neede for process module abstraction.
  vtkGetMacro(LODUpdateSuppressorID,vtkClientServerID);
  vtkGetMacro(LODMapperID,vtkClientServerID);
  vtkGetMacro(LODDeciID,vtkClientServerID);
  
  //BTX
  // Description:
  // Returns an up to data information object.
  // Do not keep a reference to this object.
  vtkPVLODPartDisplayInformation* GetInformation();
  //ETX

  // Description:
  // Toggle the visibility of the point labels.  This feature only works
  // in single-process mode.  To be changed/moved when we rework 2D rendering
  // in ParaView.
  void SetPointLabelVisibility(int val);
  
protected:
  vtkPVLODPartDisplay();
  ~vtkPVLODPartDisplay();

  int LODResolution;
  
  vtkClientServerID LODUpdateSuppressorID;
  vtkClientServerID LODMapperID;
  vtkClientServerID LODDeciID;

  // Adding point labelling back in.  This only works in single-process mode.
  // This code will be changed/moved when we rework 2D rendering in ParaView.
  vtkClientServerID PointLabelMapperID;
  vtkClientServerID PointLabelActorID;
  
  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  virtual void CreateParallelTclObjects(vtkPVProcessModule *pm);

  vtkPVLODPartDisplayInformation* Information;
  int InformationIsValid;

  vtkPVLODPartDisplay(const vtkPVLODPartDisplay&); // Not implemented
  void operator=(const vtkPVLODPartDisplay&); // Not implemented
};

#endif
