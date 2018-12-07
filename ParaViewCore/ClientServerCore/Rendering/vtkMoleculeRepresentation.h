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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int ProcessViewRequest(vtkInformationRequestKey* requestType, vtkInformation* inputInfo,
    vtkInformation* outputInfo) override;

  void SetVisibility(bool value) override;

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
  vtkDataObject* GetRenderedDataObject(int port) override;

  void MarkModified() override;

protected:
  vtkMoleculeRepresentation();
  ~vtkMoleculeRepresentation() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  bool AddToView(vtkView* view) override;
  bool RemoveFromView(vtkView* view) override;

  bool IsCached(double cache_key) override;

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
