/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositePartDisplay.h
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
// .NAME vtkPVCompositePartDisplay - Creates collection filters.
// .SECTION Description
// In addition to the LOD pipeline added by the super class,
// this subclass adds a collection filter to render locally.
// This class is also used for client server.

#ifndef __vtkPVCompositePartDisplay_h
#define __vtkPVCompositePartDisplay_h

#include "vtkPVLODPartDisplay.h"

class vtkDataSet;
class vtkPVApplication;
class vtkPVDataInformation;
class vtkPVLODPartDisplayInformation;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkPVPart;

class VTK_EXPORT vtkPVCompositePartDisplay : public vtkPVLODPartDisplay
{
public:
  static vtkPVCompositePartDisplay* New();
  vtkTypeRevisionMacro(vtkPVCompositePartDisplay, vtkPVLODPartDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enables or disables the collection filter.
  void SetCollectionDecision(int val);
  vtkGetMacro(CollectionDecision, int);
  virtual void SetLODCollectionDecision(int val);
  vtkGetMacro(LODCollectionDecision, int);

  // Description:
  // Connect the geometry filter to the display pipeline.
  virtual void ConnectToGeometry(vtkClientServerID geometryID);
          
  // Description:
  // Collection filters for both levels of detail.
  vtkGetMacro(CollectID, vtkClientServerID);
  vtkGetMacro(LODCollectID, vtkClientServerID);

  // Description:
  // This is a little different than superclass 
  // because it updates the geometry if it is out of date.
  //  Collection flag gets turned off if it needs to update.
  vtkPVLODPartDisplayInformation* GetInformation();
    
protected:
  vtkPVCompositePartDisplay();
  ~vtkPVCompositePartDisplay();
  
  int CollectionDecision;
  int LODCollectionDecision;

  vtkClientServerID CollectID;
  vtkClientServerID LODCollectID;
  
  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  virtual void CreateParallelTclObjects(vtkPVApplication *pvApp);

  vtkPVCompositePartDisplay(const vtkPVCompositePartDisplay&); // Not implemented
  void operator=(const vtkPVCompositePartDisplay&); // Not implemented
};

#endif
