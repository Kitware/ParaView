/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGlyphFilterLegacy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVGlyphFilterLegacy
 * @brief   extended API for vtkGlyph3D for better control
 * over glyph placement.
 *
 *
 * vtkPVGlyphFilterLegacy extends vtkGlyph3D for adding control over which points are
 * glyphed using \c GlyphMode. Three modes are now provided:
 * \li ALL_POINTS: all points in the input dataset are glyphed. This same as using
 * vtkGlyph3D directly.
 *
 * \li EVERY_NTH_POINT: every n-th point in the input dataset when iterated
 * through the input points sequentially is glyphed. For composite datasets,
 * the counter resets every on block. In parallel, independent counter is used
 * on each rank. Use \c Stride to control now may points to skip.
 *
 * \li SPATIALLY_UNIFORM_DISTRIBUTION: points close to a randomly sampled spatial
 * distribution of points are glyphed. \c Seed controls the seed point for the random
 * number generator (vtkMinimalStandardRandomSequence). \c MaximumNumberOfSamplePoints
 * can be used to limit the number of sample points used for random sampling. This
 * doesn't not equal the number of points actually glyphed, since that depends on
 * several factors. In parallel, this filter ensures that spatial bounds are collected
 * across all ranks for generating identical sample points.
*/

#ifndef vtkPVGlyphFilterLegacy_h
#define vtkPVGlyphFilterLegacy_h

#include "vtkGlyph3D.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVGlyphFilterLegacy : public vtkGlyph3D
{
public:
  enum GlyphModeType
  {
    ALL_POINTS,
    EVERY_NTH_POINT,
    SPATIALLY_UNIFORM_DISTRIBUTION
  };

  vtkTypeMacro(vtkPVGlyphFilterLegacy, vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVGlyphFilterLegacy* New();

  //@{
  /**
   * Get/Set the vtkMultiProcessController to use for parallel processing.
   * By default, the vtkMultiProcessController::GetGlobalController() will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  //@{
  /**
   * Set/Get the mode at which glyphs will be generated.
   */
  vtkSetClampMacro(GlyphMode, int, ALL_POINTS, SPATIALLY_UNIFORM_DISTRIBUTION);
  vtkGetMacro(GlyphMode, int);
  //@}

  //@{
  /**
   * Set/Get the stride at which to glyph the dataset.
   * Note, only applicable with EVERY_NTH_POINT GlyphMode.
   */
  vtkSetClampMacro(Stride, int, 1, VTK_INT_MAX);
  vtkGetMacro(Stride, int);
  //@}

  //@{
  /**
   * Set/Get Seed used for generating a spatially uniform distribution.
   */
  vtkSetMacro(Seed, int);
  vtkGetMacro(Seed, int);
  //@}

  //@{
  /**
   * Set/Get maximum number of sample points to use to sample the space when
   * GlyphMode is set to SPATIALLY_UNIFORM_DISTRIBUTION.
   */
  vtkSetClampMacro(MaximumNumberOfSamplePoints, int, 1, VTK_INT_MAX);
  vtkGetMacro(MaximumNumberOfSamplePoints, int);
  //@}

  //@{
  /**
   * Overridden to create output data of appropriate type.
   */
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkPVGlyphFilterLegacy();
  ~vtkPVGlyphFilterLegacy() override;
  //@}

  // Standard Pipeline methods
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Returns 1 if point is to be glyphed, otherwise returns 0.
   */
  int IsPointVisible(vtkDataSet* ds, vtkIdType ptId) override;

  /**
   * Returns true if input Scalars and Vectors are compatible, otherwise returns 0.
   */
  bool IsInputArrayToProcessValid(vtkDataSet* input);

  /**
   * Returns true if input Scalars and Vectors are cell attributes, otherwise returns 0.
   */
  bool UseCellCenters(vtkDataSet* input);

  /**
   * Returns true if input scalars are used for glyphing.
   */
  bool NeedsScalars();

  /**
   * Returns true if input vectors are used for glyphing.
   */
  bool NeedsVectors();

  /**
   * Method called in RequestData() to do the actual data processing. This will
   * apply a Cell Centers before the Glyph. The \c input, filling up the \c output
   * based on the filter parameters.
   */
  virtual bool ExecuteWithCellCenters(
    vtkDataSet* input, vtkInformationVector* sourceVector, vtkPolyData* output);

  int GlyphMode;
  int MaximumNumberOfSamplePoints;
  int Seed;
  int Stride;
  vtkMultiProcessController* Controller;

private:
  vtkPVGlyphFilterLegacy(const vtkPVGlyphFilterLegacy&) = delete;
  void operator=(const vtkPVGlyphFilterLegacy&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
