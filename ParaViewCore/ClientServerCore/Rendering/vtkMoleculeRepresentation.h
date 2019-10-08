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

/**
 * @class vtkMoleculeRepresentation
 * @brief representation for showing vtkMolecules
 */
#ifndef vtkMoleculeRepresentation_h
#define vtkMoleculeRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"

#include "vtkNew.h" // For vtkNew.

class vtkActor;
class vtkMolecule;
class vtkMoleculeMapper;
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

  /**
   * Set the opacity on the corresponding actor property.
   */
  virtual void SetOpacity(double val);

  /**
   * Set if scalars are mapped through a color-map or are used directly as colors.
   * @see vtkScalarsToColors::MapScalars
   */
  void SetMapScalars(bool map);

  /**
   * No-op. For compatibility with vtkPVCompositeRepresentation, which calls
   * SetRepresentation on it's subproxies.
   */
  void SetRepresentation(const char*) {}

  /**
   * Return the data object that is rendered from the given input port.
   */
  vtkDataObject* GetRenderedDataObject(int port) override;

  //@{
  /**
   * Forward custom atom/bonds rendering parameters to the mapper.
   */
  void SetAtomicRadiusType(int type);
  void SetAtomicRadiusScaleFactor(double factor);
  void SetBondRadius(double factor);
  void SetBondColorMode(int mode);
  void SetBondColor(double color[3]);
  void SetBondColor(double r, double g, double b);
  void SetUseMultiCylindersForBonds(bool use);
  void SetRenderAtoms(bool render);
  void SetRenderBonds(bool render);
  void SetAtomicRadiusArray(const char* name);
  //@}

protected:
  vtkMoleculeRepresentation();
  ~vtkMoleculeRepresentation() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  bool AddToView(vtkView* view) override;
  bool RemoveFromView(vtkView* view) override;

  void SyncMapper();
  void UpdateColoringParameters();

  vtkActor* Actor;
  vtkMoleculeMapper* Mapper;

  vtkNew<vtkMolecule> Molecule;

  int MoleculeRenderMode;
  bool UseCustomRadii;

  double DataBounds[6];

private:
  vtkMoleculeRepresentation(const vtkMoleculeRepresentation&) = delete;
  void operator=(const vtkMoleculeRepresentation&) = delete;
};

#endif // vtkMoleculeRepresentation_h
