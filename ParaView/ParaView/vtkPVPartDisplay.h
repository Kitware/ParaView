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


#include "vtkObject.h"

#include "vtkClientServerID.h" // Needed for PropID ...

class vtkDataSet;
class vtkPVApplication;
class vtkPVDataInformation;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkPVPart;
class vtkPVColorMap;

class VTK_EXPORT vtkPVPartDisplay : public vtkObject
{
public:
  static vtkPVPartDisplay* New();
  vtkTypeRevisionMacro(vtkPVPartDisplay, vtkObject);
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
  // This just sets the color of the property.
  // you also have to set scalar visiblity to off.
  virtual void SetColor(float r, float g, float b);

  // Description:
  // This also creates the vtk objects for the composite.
  // (actor, mapper, ...)
  virtual void SetPVApplication(vtkPVApplication *pvApp);
  vtkGetObjectMacro(PVApplication,vtkPVApplication);

  //BTX
  // Description:
  // Connect the geometry filter to the display pipeline.
  virtual void ConnectToGeometry(vtkClientServerID );
  //ETX

  // Description:
  // This method updates the piece that has been assigned to this process.
  virtual void Update();

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);  

  //===================

  vtkGetObjectMacro(Mapper, vtkPolyDataMapper);

          
  //=============================================================== 
  // Description:
  // These access methods are neede for process module abstraction.
  vtkProperty *GetProperty() { return this->Property;}
  vtkProp *GetProp() { return this->Prop;}
  vtkGetMacro(PropID, vtkClientServerID);
  vtkGetMacro(PropertyID, vtkClientServerID);
  vtkGetMacro(MapperID, vtkClientServerID);

  // Description:
  // Not referenced counted.  I might get rid of this reference later.
  virtual void SetPart(vtkPVPart* part) {this->Part = part;}
  vtkPVPart* GetPart() {return this->Part;}

  // Description:
  // PVSource calls this when it gets modified.
  void InvalidateGeometry();

protected:
  vtkPVPartDisplay();
  ~vtkPVPartDisplay();
  
  virtual void RemoveAllCaches();

  // I might get rid of this reference.
  vtkPVPart* Part;

  vtkPVApplication *PVApplication;

  int DirectColorFlag;
  int Visibility;

  // Problems with vtkLODActor led me to use these.
  vtkProperty *Property;
  vtkProp *Prop;

  vtkClientServerID PropID;
  vtkClientServerID PropertyID;
  vtkClientServerID MapperID;
  vtkClientServerID UpdateSuppressorID;

  // Here to create unique names.
  int InstanceCount;

  int GeometryIsValid;

  vtkPolyDataMapper *Mapper;

  // This method gets called by SetPVApplication.
  virtual void CreateParallelTclObjects(vtkPVApplication *pvApp);

  vtkPVPartDisplay(const vtkPVPartDisplay&); // Not implemented
  void operator=(const vtkPVPartDisplay&); // Not implemented
};

#endif
