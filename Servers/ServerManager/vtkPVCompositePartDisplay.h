/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositePartDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
  // Collection filters for both levels of detail.
  vtkGetMacro(CollectID, vtkClientServerID);
  vtkGetMacro(LODCollectID, vtkClientServerID);

  //BTX
  // Description:
  // This is a little different than superclass 
  // because it updates the geometry if it is out of date.
  //  Collection flag gets turned off if it needs to update.
  vtkPVLODPartDisplayInformation* GetInformation();
  //ETX
    
protected:
  // Create the CollectID object
  vtkClientServerID CreateCollectionFilter(vtkPVProcessModule*);
  
  vtkPVCompositePartDisplay();
  ~vtkPVCompositePartDisplay();
  
  int CollectionDecision;
  int LODCollectionDecision;

  vtkClientServerID CollectID;
  vtkClientServerID LODCollectID;
  
  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  virtual void CreateParallelTclObjects(vtkPVProcessModule *pm);

  vtkPVCompositePartDisplay(const vtkPVCompositePartDisplay&); // Not implemented
  void operator=(const vtkPVCompositePartDisplay&); // Not implemented
};

#endif
