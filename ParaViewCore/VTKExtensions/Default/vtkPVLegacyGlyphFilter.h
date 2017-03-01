/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLegacyGlyphFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVLegacyGlyphFilter
 * @brief   Glyph filter
 *
 *
 * This is a subclass of vtkGlyph3D that allows selection of input scalars
*/

#ifndef vtkPVLegacyGlyphFilter_h
#define vtkPVLegacyGlyphFilter_h

#include "vtkGlyph3D.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkMaskPoints;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVLegacyGlyphFilter : public vtkGlyph3D
{
public:
  vtkTypeMacro(vtkPVLegacyGlyphFilter, vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkPVLegacyGlyphFilter* New();

  //@{
  /**
   * Limit the number of points to glyph
   */
  vtkSetMacro(MaximumNumberOfPoints, int);
  vtkGetMacro(MaximumNumberOfPoints, int);
  //@}

  //@{
  /**
   * Get the number of processes used to run this filter.
   */
  vtkGetMacro(NumberOfProcesses, int);
  //@}

  //@{
  /**
   * Set/get whether to mask points
   */
  void SetUseMaskPoints(int useMaskPoints);
  vtkGetMacro(UseMaskPoints, int);
  //@}

  //@{
  /**
   * Set/get flag to cause randomization of which points to mask.
   */
  void SetRandomMode(int mode);
  int GetRandomMode();
  //@}

  /**
   * In processing composite datasets, will check if a point
   * is visible as long as the dataset being process if a
   * vtkUniformGrid.
   */
  virtual int IsPointVisible(vtkDataSet* ds, vtkIdType ptId) VTK_OVERRIDE;

  void SetKeepRandomPoints(int keepRandomPoints);
  vtkGetMacro(KeepRandomPoints, int);

protected:
  vtkPVLegacyGlyphFilter();
  ~vtkPVLegacyGlyphFilter();

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestCompositeData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive() VTK_OVERRIDE;

  vtkIdType GatherTotalNumberOfPoints(vtkIdType localNumPts);

  /**
   * This is a generic function that can be called per
   * block of the dataset to calculate indices of points
   * to be glyphed in the block
   */
  void CalculatePtsToGlyph(double PtsNotBlanked);

  vtkMaskPoints* MaskPoints;
  int MaximumNumberOfPoints;
  int NumberOfProcesses;
  int UseMaskPoints;
  int InputIsUniformGrid;

  vtkIdType BlockGlyphAllPoints;
  vtkIdType BlockMaxNumPts;
  vtkIdType BlockOnRatio;
  vtkIdType BlockPointCounter;
  vtkIdType BlockNextPoint;
  vtkIdType BlockNumGlyphedPts;

  std::vector<vtkIdType> RandomPtsInDataset;

  int RandomMode;

  virtual void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;

  int KeepRandomPoints;
  vtkIdType MaximumNumberOfPointsOld;

private:
  vtkPVLegacyGlyphFilter(const vtkPVLegacyGlyphFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVLegacyGlyphFilter&) VTK_DELETE_FUNCTION;

public:
  enum CommunicationIds
  {
    GlyphNPointsGather = 1000,
    GlyphNPointsScatter
  };
};

#endif
