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
// .NAME vtkPVPartDisplay - This contains all the LOD, mapper and actor stuff.
// .SECTION Description
// I am separating parts into data and rendering stuff.  
// I intend to subclass this object for different render modules.
// I decided not to include the geometry filter because
// vtkPVData controls so many variables of the geometry filter. 

#ifndef __vtkPVPartDisplay_h
#define __vtkPVPartDisplay_h


#include "vtkKWObject.h"

class vtkDataSet;
class vtkPVApplication;
class vtkPVDataInformation;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkPVPart;

class VTK_EXPORT vtkPVPartDisplay : public vtkKWObject
{
public:
  static vtkPVPartDisplay* New();
  vtkTypeRevisionMacro(vtkPVPartDisplay, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enables or disables the collection filter.
  void SetCollectionDecision(int val);
  vtkGetMacro(CollectionDecision, int);
  void SetLODCollectionDecision(int val);
  vtkGetMacro(LODCollectionDecision, int);

  // Description:
  // Set the number of bins per axes on the quadric decimation filter.
  void SetLODResolution(int res);

  // Description:
  // Toggles the mappers to use immediate mode rendering or display lists.
  void SetUseImmediateMode(int val);

  // Description:
  // Turns visibilioty on or off.
  void SetVisibility(int v);
  vtkGetMacro(Visibility,int);

  // Description:
  // Change the color mode to map scalars or 
  // use unsigned char arrays directly.
  // MapScalarsOff only works when coloring by an array 
  // on unsigned chars with 1 or 3 components.
  void SetDirectColorFlag(int val);
  vtkGetMacro(DirectColorFlag, int);

  // Description:
  // This also creates the parallel vtk objects for the composite.
  // (actor, mapper, ...)
  void SetPVApplication(vtkPVApplication *pvApp);
  void SetApplication(vtkKWApplication *)
    {
    vtkErrorMacro("vtkPVPartDisplay::SetApplication should not be used. Use SetPVApplcation instead.");
    }

  // Description:
  // Connect the geometry filter to the display pipeline.
  void ConnectToGeometry(char* geometryTclName);

  // Description:
  // This method updates the piece that has been assigned to this process.
  // It also gathers the data information.
  void Update();
  void ForceUpdate(vtkPVApplication* pvApp);
  
  //===================
          
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  vtkGetObjectMacro(Mapper, vtkPolyDataMapper);

  // Description:
  // Tcl name of the actor across all processes.
  vtkGetStringMacro(PropTclName);  
          
  //=============================================================== 
  // Description:
  // These access methods are neede for process module abstraction.
  vtkGetStringMacro(UpdateSuppressorTclName);
  vtkGetStringMacro(LODUpdateSuppressorTclName);
  vtkGetStringMacro(MapperTclName);
  vtkGetStringMacro(LODMapperTclName);
  vtkGetStringMacro(LODDeciTclName);
  vtkGetStringMacro(PropertyTclName);
  vtkGetStringMacro(CollectTclName);
  vtkGetStringMacro(LODCollectTclName);
  vtkProperty *GetProperty() { return this->Property;}
  vtkProp *GetProp() { return this->Prop;}
    
  // Description:
  // For flip books.
  void RemoveAllCaches();
  void CacheUpdate(int idx, int total);

  // Description:
  // Not referenced counted.  I might get rid of this reference later.
  void SetPart(vtkPVPart* part) {this->Part = part;}
  vtkPVPart* GetPart() {return this->Part;}

protected:
  vtkPVPartDisplay();
  ~vtkPVPartDisplay();
  
  // I might get rid of this reference.
  vtkPVPart* Part;

  int CollectionDecision;
  int LODCollectionDecision;
  int DirectColorFlag;
  int Visibility;

  // Problems with vtkLODActor led me to use these.
  vtkProperty *Property;
  vtkProp *Prop;
        
  char *PropTclName;
  vtkSetStringMacro(PropTclName);
  
  char *PropertyTclName;
  vtkSetStringMacro(PropertyTclName);
  
  char *MapperTclName;
  vtkSetStringMacro(MapperTclName);

  char *LODMapperTclName;
  vtkSetStringMacro(LODMapperTclName);
  
  char *LODDeciTclName;
  vtkSetStringMacro(LODDeciTclName);
    
  char *UpdateSuppressorTclName;
  vtkSetStringMacro(UpdateSuppressorTclName);
  
  char *LODUpdateSuppressorTclName;
  vtkSetStringMacro(LODUpdateSuppressorTclName);
  
  char *CollectTclName;
  vtkSetStringMacro(CollectTclName);

  char *LODCollectTclName;
  vtkSetStringMacro(LODCollectTclName);
  
  // Here to create unique names.
  int InstanceCount;

  // If the data changes, we need to change to.
  vtkTimeStamp UpdateTime;

  vtkPolyDataMapper *Mapper;

  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  void CreateParallelTclObjects(vtkPVApplication *pvApp);

  vtkPVPartDisplay(const vtkPVPartDisplay&); // Not implemented
  void operator=(const vtkPVPartDisplay&); // Not implemented
};

#endif
