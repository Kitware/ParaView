/*=========================================================================

  Program:   ParaView
  Module:    vtkVolumeRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkVolumeRepresentation
 * @brief Abstract base class for volume representations. Provides some functionality common to
 * volume representations.
 *
 */
#ifndef vtkVolumeRepresentation_h
#define vtkVolumeRepresentation_h

#include "vtkPVDataRepresentation.h"

#include "vtkNew.h"
#include "vtkPVLODVolume.h"
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"        //needed for vtkSmartPointer

class vtkColorTransferFunction;
class vtkDataSet;
class vtkOutlineSource;
class vtkPiecewiseFunction;
class vtkPVLODVolume;
class vtkVolumeProperty;

class VTKREMOTINGVIEWS_EXPORT vtkVolumeRepresentation : public vtkPVDataRepresentation
{
public:
  vtkTypeMacro(vtkVolumeRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   */
  void SetVisibility(bool val) override;

  //***************************************************************************
  // Forwarded to Actor.
  void SetOrientation(double, double, double);
  void SetOrigin(double, double, double);
  void SetPickable(int val);
  void SetPosition(double, double, double);
  void SetScale(double, double, double);

  //***************************************************************************
  // Forwarded to vtkVolumeProperty and vtkProperty (when applicable).
  void SetInterpolationType(int val);
  void SetColor(vtkColorTransferFunction* lut);
  void SetScalarOpacity(vtkPiecewiseFunction* pwf);
  void SetScalarOpacityUnitDistance(double val);
  void SetMapScalars(bool);
  void SetMultiComponentsMapping(bool);

  //***************************************************************************
  // For separate opacity array support
  void SetUseSeparateOpacityArray(bool);
  void SelectOpacityArray(int, int, int, int, const char* name);
  void SelectOpacityArrayComponent(int component);

  /**
   * Provides access to the actor used by this representation.
   */
  vtkPVLODVolume* GetActor() { return this->Actor; }

protected:
  vtkVolumeRepresentation();
  ~vtkVolumeRepresentation() override;

  vtkNew<vtkVolumeProperty> Property;
  vtkSmartPointer<vtkPVLODVolume> Actor;
  vtkNew<vtkOutlineSource> OutlineSource;

  unsigned long DataSize = 0;
  double DataBounds[6];

  bool MapScalars = true;
  bool MultiComponentsMapping = false;

  bool UseSeparateOpacityArray = false;
  std::string OpacityArrayName;
  int OpacityArrayFieldAssociation = -1;
  int OpacityArrayComponent = -1;

  /**
   * Appends a designated opacity component to a scalar array used for color/opacity mapping.
   */
  bool AppendOpacityComponent(vtkDataSet* dataset);

private:
  vtkVolumeRepresentation(const vtkVolumeRepresentation&) = delete;
  void operator=(const vtkVolumeRepresentation&) = delete;
};

#endif // vtkVolumeRepresentation_h
