/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPartDisplay.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include "vtkClientServerStream.h"

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

  // Description:
  // Connect the geometry filter to the display pipeline.
  virtual void ConnectToGeometry(vtkClientServerID );

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
//  vtkClientServerID GetPropID() { return this->PropID;}

  // Description:
  // Not referenced counted.  I might get rid of this reference later.
  virtual void SetPart(vtkPVPart* part) {this->Part = part;}
  vtkPVPart* GetPart() {return this->Part;}

  // Description:
  // PVSource calls this when it gets modified.
  void InvalidateGeometry();

  char* UpdateSuppressorTclName;
  const char* GetPropertyTclName() 
    {
      return "";
    }
  const char* GetMapperTclName()
    {
      return "";
    }
  const char* GetPropTclName()
    {
      return "";
    }
  char* PropTclName;
  char* MapperTclName;
  char* PropertyTclName;
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
