/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTMultiDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkSMIceTMultiDisplayProxy - For tiled display driven by ICE-T.
// .SECTION Description
// Adds some slight functionality for doing multi tile displays with ICE-T.
// Mostly just handles cases where there is a lot of data being rendered
// (i.e. don't necessarily collect LDO on node zero).

#ifndef __vtkSMIceTMultiDisplayProxy_h
#define __vtkSMIceTMultiDisplayProxy_h

#include "vtkSMMultiDisplayProxy.h"

class VTK_EXPORT vtkSMIceTMultiDisplayProxy : public vtkSMMultiDisplayProxy
{
public:
  vtkTypeRevisionMacro(vtkSMIceTMultiDisplayProxy, vtkSMMultiDisplayProxy);
  static vtkSMIceTMultiDisplayProxy *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Overridden to only collect LOD when the data is less than some
  // threshold.
  virtual void SetCollectionDecision(int);
  virtual void SetLODCollectionDecision(int);

  // Description:
  // When on, geometry collection is suppressed and instead outlines are
  // collected.
  vtkSetMacro(SuppressGeometryCollection, int);
  vtkGetMacro(SuppressGeometryCollection, int);

protected:
  vtkSMIceTMultiDisplayProxy();
  ~vtkSMIceTMultiDisplayProxy();

  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // Sets the local collectors inputs to be bounding boxes.
  virtual void SetupPipeline();
  virtual void SetupDefaults();

  vtkSMProxy *OutlineFilterProxy;
  vtkSMProxy *OutlineCollectProxy;
  vtkSMProxy *OutlineUpdateSuppressorProxy;

  int SuppressGeometryCollection;

private:
  vtkSMIceTMultiDisplayProxy(const vtkSMIceTMultiDisplayProxy &);  // Not implemented
  void operator=(const vtkSMIceTMultiDisplayProxy &);  // Not implemented
};

#endif //__vtkSMIceTMultiDisplayProxy_h
