/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPartDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPartDisplay - Superclass for actor/mapper control.
// .SECTION Description
// This is a superclass for objects which display PVParts.
// vtkPVRenderModules create displays.  This class is not meant to be
// used directly, but it implements the simplest serial display
// which has no levels of detail.

#ifndef __vtkPVPartDisplay_h
#define __vtkPVPartDisplay_h


#include "vtkPVDisplay.h"

#include "vtkClientServerID.h" // Needed for PropID ...

class vtkDataSet;
class vtkPVApplication;
class vtkPVDataInformation;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkPVPart;
class vtkPVColorMap;
class vtkVolume;
class vtkVolumeProperty;
class vtkPiecewiseFunction;
class vtkColorTransferFunction;
class vtkUnstructuredGridVolumeRayCastMapper;
class vtkPVArrayInformation;

class VTK_EXPORT vtkPVPartDisplay : public vtkPVDisplay
{
public:
  static vtkPVPartDisplay* New();
  vtkTypeRevisionMacro(vtkPVPartDisplay, vtkPVDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Toggles the mappers to use immediate mode rendering or display lists.
  virtual void SetUseImmediateMode(int val);

  // Description:
  // Turns visibilioty on or off.
  virtual void SetVisibility(int v);
  vtkGetMacro(Visibility,int);

  // Description:
  // Change the color mode to map scalars or 
  // use unsigned char arrays directly.
  // MapScalarsOff only works when coloring by an array 
  // on unsigned chars with 1 or 3 components.
  virtual void SetDirectColorFlag(int val);
  vtkGetMacro(DirectColorFlag, int);
  virtual void SetScalarVisibility(int val);
  virtual void ColorByArray(vtkPVColorMap *colorMap, int field);

  // Description:
  // Option to use a 1d texture map for the attribute color.
  virtual void SetInterpolateColorsFlag(int val);
  vtkGetMacro(InterpolateColorsFlag, int);

  // Description:
  // This just sets the color of the property.
  // you also have to set scalar visiblity to off.
  virtual void SetColor(float r, float g, float b);

  // Description:
  // This also creates the vtk objects for the composite.
  // (actor, mapper, ...)
  virtual void SetPVApplication(vtkPVApplication *pvApp);
  vtkGetObjectMacro(PVApplication,vtkPVApplication);

  // Description:
  // Connect the VTK data object to the display pipeline.
  virtual void SetInput(vtkPVPart* input);

  // Description:
  // This method updates the piece that has been assigned to this process.
  virtual void Update();

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);  

  //BTX
  // Description:
  // Return a pointer to the mapper (on the client side)
  vtkGetObjectMacro(Mapper, vtkPolyDataMapper);
          
  // Description:
  // Return a pointer to the property (on the client side)
  vtkProperty *GetProperty() { return this->Property;}

  // Description:
  // Return a pointer to the actor (on the client side)
  vtkProp *GetProp() { return this->Prop;}
  //ETX

  // Description:
  // Return the id of the actor (on the server side)
  vtkGetMacro(PropID, vtkClientServerID);

  // Description:
  // Return the id of the property (on the server side)
  vtkGetMacro(PropertyID, vtkClientServerID);

  // Description:
  // Return the id of the mapper (on the server side)
  vtkGetMacro(MapperID, vtkClientServerID);

  // Description:
  // Return the id of the volume (on the server side)
  vtkGetMacro(VolumeID, vtkClientServerID);

  // Description:
  // Return the id of the volume opacity tfun (on the server side)
  vtkGetMacro(VolumeOpacityID, vtkClientServerID);

  // Description:
  // Return the id of the volume color tfun (on the server side)
  vtkGetMacro(VolumeColorID, vtkClientServerID);

  // Description:
  // Return the id of the volume property (on the server side)
  vtkGetMacro(VolumePropertyID, vtkClientServerID);

  //BTX
  // Description:
  // Return the actual objects
  vtkVolume *GetVolume() { return this->Volume; };
  vtkPiecewiseFunction *GetVolumeOpacity() {return this->VolumeOpacity;};
  vtkColorTransferFunction *GetVolumeColor() {return this->VolumeColor;};
  //ETX
  
  // Description:
  // Not referenced counted.  I might get rid of this reference later.
  virtual void SetPart(vtkPVPart* part) {this->Part = part;}
  vtkPVPart* GetPart() {return this->Part;}

  // Description:
  // PVSource calls this when it gets modified.
  void InvalidateGeometry();

  // Description:
  // fixme:  does this really need to be here?
  // Get the tcl name of the vtkPVGeometryFilter.
  vtkClientServerID GetGeometryID() {return this->GeometryID;}

  // Description:
  // Select the point field to use for volume rendering
  void VolumeRenderPointField(const char *name);

  // Description:
  // Turn on/off volume rendering. This controls which prop is 
  // visible since both geometric and volumetric pipelines exist
  // simultaneously.
  void VolumeRenderModeOn();
  void VolumeRenderModeOff();

  // Description:
  // Initialize the transfer functions based on the scalar range
  void ResetTransferFunctions(vtkPVArrayInformation *arrayInfo,
                              vtkPVDataInformation *dataInfo);
  void InitializeTransferFunctions(vtkPVArrayInformation *arrayInfo, 
                                   vtkPVDataInformation *dataInfo);
  
protected:
  vtkPVPartDisplay();
  ~vtkPVPartDisplay();
  
  virtual void RemoveAllCaches();

  // Description:
  // Sends the current stream to the client and server. 
  void SendForceUpdate();

  // I might get rid of this reference.
  vtkPVPart* Part;

  vtkPVApplication *PVApplication;

  int DirectColorFlag;
  int InterpolateColorsFlag;
  int Visibility;

  // Problems with vtkLODActor led me to use these.
  vtkProperty *Property;
  vtkProp *Prop;

  vtkClientServerID GeometryID;
  vtkClientServerID PropID;
  vtkClientServerID PropertyID;
  vtkClientServerID MapperID;
  vtkClientServerID UpdateSuppressorID;

  vtkClientServerID VolumeID;
  vtkClientServerID VolumePropertyID;
  vtkClientServerID VolumeMapperID;
  vtkClientServerID VolumeOpacityID;
  vtkClientServerID VolumeColorID;
  vtkClientServerID VolumeTetraFilterID;
  vtkClientServerID VolumeFieldFilterID;

  vtkVolume                *Volume;
  vtkPiecewiseFunction     *VolumeOpacity;
  vtkColorTransferFunction *VolumeColor;
  
  // Here to create unique names.
  int InstanceCount;

  int GeometryIsValid;

  vtkPolyDataMapper *Mapper;

  int VolumeRenderMode;
  
  // This method gets called by SetPVApplication.
  virtual void CreateParallelTclObjects(vtkPVApplication *pvApp);

  vtkPVPartDisplay(const vtkPVPartDisplay&); // Not implemented
  void operator=(const vtkPVPartDisplay&); // Not implemented
};

#endif
