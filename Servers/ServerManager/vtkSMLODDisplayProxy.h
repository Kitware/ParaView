/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLODDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLODDisplayProxy - a simple display proxy.
// .SECTION Description
#ifndef __vtkSMLODDisplayProxy_h
#define __vtkSMLODDisplayProxy_h

#include "vtkSMSimpleDisplayProxy.h"
class vtkSMProxy;
class vtkPVLODPartDisplayInformation;

class VTK_EXPORT vtkSMLODDisplayProxy : public vtkSMSimpleDisplayProxy
{
public:
  static vtkSMLODDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMLODDisplayProxy, vtkSMSimpleDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of bins per axes on the quadric decimation filter.
  virtual void SetLODResolution(int res);
  
  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);

  // Description:
  // This method calls a ForceUpdate on the UpdateSuppressor
  // if the Geometry is not valid. 
  virtual void Update();
  
  // Description:
  // Invalidates Geometry. Results in removal of any cached geometry. Also,
  // marks the current geometry as invalid, thus a subsequent call to Update
  // will result in call to ForceUpdate on the UpdateSuppressor(s), if any.
  virtual void InvalidateGeometry();

  // Description:
  // Returns an up to data information object.
  // Do not keep a reference to this object.
  virtual vtkPVLODPartDisplayInformation* GetLODInformation();

  //BTX
  enum {InformationInvalidatedEvent = 2000};
  //ETX

protected:
  vtkSMLODDisplayProxy();
  ~vtkSMLODDisplayProxy();
  
  // Description:
  // Set up the vtkUnstructuredGrid (Volume) rendering pipeline.
  virtual void SetupVolumePipeline();

  // Description:
  // Set up the PolyData rendering pipeline.
  virtual void SetupPipeline();
  virtual void SetupDefaults();

  virtual void CreateVTKObjects(int numObjects);

  vtkSMProxy *LODDecimatorProxy;
  vtkSMProxy *LODUpdateSuppressorProxy;
  vtkSMProxy *LODMapperProxy; 

  int LODResolution;
  int LODGeometryIsValid;
  int LODInformationIsValid;
  vtkPVLODPartDisplayInformation* LODInformation;  

  void InvalidateLODGeometry();

  // Calls Force Update on the LOD Update suppressor.
  void UpdateLODPipeline();

private:
  vtkSMLODDisplayProxy(const vtkSMLODDisplayProxy&); // Not implemented.
  void operator=(const vtkSMLODDisplayProxy&); // Not implemented.
};


#endif


