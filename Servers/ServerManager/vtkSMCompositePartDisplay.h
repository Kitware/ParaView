/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositePartDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCompositePartDisplay - Creates collection filters.
// .SECTION Description
// In addition to the LOD pipeline added by the super class,
// this subclass adds a collection filter to render locally.
// This class is also used for client server.

#ifndef __vtkSMCompositePartDisplay_h
#define __vtkSMCompositePartDisplay_h

#include "vtkSMLODPartDisplay.h"

class vtkDataSet;
class vtkPVDataInformation;
class vtkPVLODPartDisplayInformation;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkSMProxy;

class VTK_EXPORT vtkSMCompositePartDisplay : public vtkSMLODPartDisplay
{
public:
  static vtkSMCompositePartDisplay* New();
  vtkTypeRevisionMacro(vtkSMCompositePartDisplay, vtkSMLODPartDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enables or disables the collection filter.
  void SetCollectionDecision(int val);
  vtkGetMacro(CollectionDecision, int);
  virtual void SetLODCollectionDecision(int val);
  vtkGetMacro(LODCollectionDecision, int);

  //BTX
  // Description:
  // This is a little different than superclass 
  // because it updates the geometry if it is out of date.
  //  Collection flag gets turned off if it needs to update.
  vtkPVLODPartDisplayInformation* GetLODInformation();
  //ETX
    
protected:
  // Create the CollectID object
  void SetupCollectionFilter(vtkSMProxy*);
 
  virtual void CreateVTKObjects(int num); 
  
  vtkSMCompositePartDisplay();
  ~vtkSMCompositePartDisplay();
  
  int CollectionDecision;
  int LODCollectionDecision;

  vtkSMProxy* CollectProxy;
  vtkSMProxy* LODCollectProxy;
  
  vtkSMCompositePartDisplay(const vtkSMCompositePartDisplay&); // Not implemented
  void operator=(const vtkSMCompositePartDisplay&); // Not implemented
};

#endif
