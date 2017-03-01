/*=========================================================================

  Program:   ParaView
  Module:    vtkAMRFragmentsFilter.h

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2014 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
/**
 * @class   vtkAMRFragmentsFilter
 * @brief   A meta filter that combines
 * vtkAMRConnectivity, vtkAMRFragmentIntegration, vtkAMRDualContour,
 * vtkExtractCTHPart to allow all the fragment analysis in one easy UI
 *
 *
 *   Input 0: The AMR Volume
 *
 *   Output 0: A multiblock containing tables of fragments, one block
 *             for each requested material
*/

#ifndef vtkAMRFragmentsFilter_h
#define vtkAMRFragmentsFilter_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkTrivialProducer;
class vtkExtractCTHPart;
class vtkPVAMRDualContour;
class vtkAMRConnectivity;
class vtkPVAMRFragmentIntegration;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkAMRFragmentsFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkAMRFragmentsFilter* New();
  vtkTypeMacro(vtkAMRFragmentsFilter, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Add to list of volume arrays to find connected fragments
   */
  void AddInputVolumeArrayToProcess(const char* name);
  void ClearInputVolumeArrayToProcess();
  //@}

  //@{
  /**
   * Add to list of mass arrays
   */
  void AddInputMassArrayToProcess(const char* name);
  void ClearInputMassArrayToProcess();
  //@}

  //@{
  /**
   * Add to list of volume weighted arrays
   */
  void AddInputVolumeWeightedArrayToProcess(const char* name);
  void ClearInputVolumeWeightedArrayToProcess();
  //@}

  //@{
  /**
   * Add to list of mass weighted arrays
   */
  void AddInputMassWeightedArrayToProcess(const char* name);
  void ClearInputMassWeightedArrayToProcess();
  //@}

  //@{
  /**
   * Whether or not to extract a surface
   */
  vtkSetMacro(ExtractSurface, bool);
  vtkGetMacro(ExtractSurface, bool);
  vtkBooleanMacro(ExtractSurface, bool);
  //@}

  //@{
  /**
   * Whether or not to use water-tight algorithm for the surface
   */
  vtkSetMacro(UseWatertightSurface, bool);
  vtkGetMacro(UseWatertightSurface, bool);
  vtkBooleanMacro(UseWatertightSurface, bool);
  //@}

  //@{
  /**
   * Whether or not to perform fragment integration
   */
  vtkSetMacro(IntegrateFragments, bool);
  vtkGetMacro(IntegrateFragments, bool);
  vtkBooleanMacro(IntegrateFragments, bool);
  //@}

  //@{
  /**
   * Get / Set volume fraction value.
   */
  vtkGetMacro(VolumeFractionSurfaceValue, double);
  vtkSetMacro(VolumeFractionSurfaceValue, double);
  //@}

protected:
  vtkAMRFragmentsFilter();
  virtual ~vtkAMRFragmentsFilter();

  bool ExtractSurface;
  bool UseWatertightSurface;
  bool IntegrateFragments;
  double VolumeFractionSurfaceValue;

  vtkTrivialProducer* Producer;
  vtkExtractCTHPart* Extract;
  vtkPVAMRDualContour* Contour;
  vtkAMRConnectivity* Connectivity;
  vtkPVAMRFragmentIntegration* Integration;

  virtual int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;
  virtual int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkAMRFragmentsFilter(const vtkAMRFragmentsFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAMRFragmentsFilter&) VTK_DELETE_FUNCTION;
};

#endif /* vtkAMRFragmentsFilter_h */
