/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODPartDisplay.h
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
// .NAME vtkPVLODPartDisplay - This contains all the LOD, mapper and actor stuff.
// .SECTION Description
// This is the part displays for serial execution of paraview.
// I handles all of the decimation levels of detail.


#ifndef __vtkPVLODPartDisplay_h
#define __vtkPVLODPartDisplay_h


#include "vtkPVPartDisplay.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID
class vtkDataSet;
class vtkPVApplication;
class vtkPVLODPartDisplayInformation;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkPVPart;

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
  virtual void ColorByArray(vtkPVColorMap *colorMap, int field);


  // Description:
  // Connect the geometry filter to the display pipeline.
  virtual void ConnectToGeometry(vtkClientServerID);

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
  
  vtkGetStringMacro(LODUpdateSuppressorTclName);
  vtkGetStringMacro(LODMapperTclName);
  vtkGetStringMacro(LODDeciTclName);
    
  // Description:
  // Returns an up to data information object.
  // Do not keep a reference to this object.
  vtkPVLODPartDisplayInformation* GetInformation();

protected:
  vtkPVLODPartDisplay();
  ~vtkPVLODPartDisplay();

  int LODResolution;
  
  vtkClientServerID LODUpdateSuppressorID;
  vtkClientServerID LODMapperID;
  vtkClientServerID LODDeciID;
  
  char *LODMapperTclName;
  vtkSetStringMacro(LODMapperTclName);
  
  char *LODDeciTclName;
  vtkSetStringMacro(LODDeciTclName);
    
  char *LODUpdateSuppressorTclName;
  vtkSetStringMacro(LODUpdateSuppressorTclName);
  
  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  virtual void CreateParallelTclObjects(vtkPVApplication *pvApp);

  vtkPVLODPartDisplayInformation* Information;
  int InformationIsValid;

  vtkPVLODPartDisplay(const vtkPVLODPartDisplay&); // Not implemented
  void operator=(const vtkPVLODPartDisplay&); // Not implemented
};

#endif
