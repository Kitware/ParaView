/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCompositeDisplayProxy- a simple display proxy.
// .SECTION Description

#ifndef __vtkSMCompositeDisplayProxy_h
#define __vtkSMCompositeDisplayProxy_h

#include "vtkSMLODDisplayProxy.h"
class vtkPVLODPartDisplayInformation;
class VTK_EXPORT vtkSMCompositeDisplayProxy : public vtkSMLODDisplayProxy
{
public:
  static vtkSMCompositeDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMCompositeDisplayProxy, vtkSMLODDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enables or disables the collection filter.
  void SetCollectionDecision(int val);
  vtkGetMacro(CollectionDecision, int);
  virtual void SetLODCollectionDecision(int val);
  vtkGetMacro(LODCollectionDecision, int);
  //BTX
  enum MoveMode {PASS_THROUGH=0, COLLECT, CLONE};
  enum Server { CLIENT=0, DATA_SERVER, RENDER_SERVER};
  //ETX

  //BTX
  // Description:
  // This is a little different than superclass 
  // because it updates the geometry if it is out of date.
  //  Collection flag gets turned off if it needs to update.
  virtual vtkPVLODPartDisplayInformation* GetLODInformation();
  //ETX
protected:
  vtkSMCompositeDisplayProxy();
  ~vtkSMCompositeDisplayProxy();

  virtual void SetupPipeline();
  virtual void SetupDefaults();


  virtual void CreateVTKObjects(int numObjects);
  void SetupCollectionFilter(vtkSMProxy* collectProxy);

  vtkSMProxy* CollectProxy;
  vtkSMProxy* LODCollectProxy;

  int CollectionDecision;
  int LODCollectionDecision;

private:
  vtkSMCompositeDisplayProxy(const vtkSMCompositeDisplayProxy&); // Not implemented.
  void operator=(const vtkSMCompositeDisplayProxy&); // Not implemented.
};

#endif

