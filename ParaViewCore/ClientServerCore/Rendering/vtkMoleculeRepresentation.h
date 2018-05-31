/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkMoleculeRepresentation_h
#define vtkMoleculeRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"

#include "vtkNew.h" // For vtkNew.

class vtkActor;
class vtkMolecule;
class vtkMoleculeMapper;
class vtkPVCacheKeeper;
class vtkScalarsToColors;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkMoleculeRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkMoleculeRepresentation* New();
  vtkTypeMacro(vtkMoleculeRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  int ProcessViewRequest(vtkInformationRequestKey* requestType, vtkInformation* inputInfo,
    vtkInformation* outputInfo) VTK_OVERRIDE;

  void SetVisibility(bool value) VTK_OVERRIDE;

  vtkGetMacro(MoleculeRenderMode, int) void SetMoleculeRenderMode(int mode);

  vtkGetMacro(UseCustomRadii, bool) void SetUseCustomRadii(bool val);

  void SetLookupTable(vtkScalarsToColors* lut);

  //***************************************************************************
  // Forwarded to Actor->GetProperty()
  virtual void SetOpacity(double val);

  // Description:
  // No-op. For compatibility with vtkPVCompositeRepresentation, which calls
  // SetRepresentation on it's subproxies.
  void SetRepresentation(const char*) {}

  // Description:
  // Returns the data object that is rendered from the given input port.
  vtkDataObject* GetRenderedDataObject(int port) VTK_OVERRIDE;

  void MarkModified() VTK_OVERRIDE;

protected:
  vtkMoleculeRepresentation();
  ~vtkMoleculeRepresentation() override;

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  bool AddToView(vtkView* view) VTK_OVERRIDE;
  bool RemoveFromView(vtkView* view) VTK_OVERRIDE;

  bool IsCached(double cache_key) VTK_OVERRIDE;

  void SyncMapper();
  void UpdateColoringParameters();

  vtkActor* Actor;
  vtkMoleculeMapper* Mapper;

  vtkNew<vtkPVCacheKeeper> CacheKeeper;
  vtkNew<vtkMolecule> DummyMolecule;

  int MoleculeRenderMode;
  bool UseCustomRadii;

  double DataBounds[6];

private:
  vtkMoleculeRepresentation(const vtkMoleculeRepresentation&) = delete;
  void operator=(const vtkMoleculeRepresentation&) = delete;
};

#endif // vtkMoleculeRepresentation_h
