/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGlyphFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGlyphFilter - extended API for vtkGlyph3D for better control
// over glyph placement.
//
// .SECTION Description
// vtkPVGlyphFilter extends vtkGlyph3D for adding control over which points are
// glyphed using \c GlyphMode. Three modes are now provided:
// \li ALL_POINTS: all points in the input dataset are glyphed. This same as using
// vtkGlyph3D directly.
//
// \li EVERY_NTH_POINT: every n-th point in the input dataset when iterated
// through the input points sequentially is glyphed. For composite datasets,
// the counter resets every on block. In parallel, independent counter is used
// on each rank. Use \c Stride to control now may points to skip.
//
// \li SPATIALLY_UNIFORM_DISTRIBUTION: points close to a randomly sampled spatial
// distribution of points are glyphed. \c Seed controls the seed point for the random
// number generator (vtkMinimalStandardRandomSequence). \c MaximumNumberOfSamplePoints
// can be used to limit the number of sample points used for random sampling. This
// doesn't not equal the number of points actually glyphed, since that depends on
// several factors. In parallel, this filter ensures that spatial bounds are collected
// across all ranks for generating identical sample points.

#ifndef __vtkPVGlyphFilter_h
#define __vtkPVGlyphFilter_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkGlyph3D.h"

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVGlyphFilter : public vtkGlyph3D
{
public:
  enum GlyphModeType
    {
    ALL_POINTS,
    EVERY_NTH_POINT,
    SPATIALLY_UNIFORM_DISTRIBUTION
    };

  vtkTypeMacro(vtkPVGlyphFilter,vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVGlyphFilter *New();

  // Description:
  // Get/Set the vtkMultiProcessController to use for parallel processing.
  // By default, the vtkMultiProcessController::GetGlobalController() will be used.
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Set/Get the mode at which glyphs will be generated.
  vtkSetClampMacro(GlyphMode, int, ALL_POINTS, SPATIALLY_UNIFORM_DISTRIBUTION);
  vtkGetMacro(GlyphMode, int);

  // Description:
  // Set/Get the stride at which to glyph the dataset.
  // Note, only applicable with EVERY_NTH_POINT GlyphMode.
  vtkSetClampMacro(Stride, int, 1, VTK_INT_MAX);
  vtkGetMacro(Stride, int);

  // Description:
  // Set/Get Seed used for generating a spatially uniform distribution.
  vtkSetMacro(Seed, int);
  vtkGetMacro(Seed, int);

  // Description:
  // Set/Get maximum number of sample points to use to sample the space when
  // GlyphMode is set to SPATIALLY_UNIFORM_DISTRIBUTION.
  vtkSetClampMacro(MaximumNumberOfSamplePoints, int, 1, VTK_INT_MAX);
  vtkGetMacro(MaximumNumberOfSamplePoints, int);

  // Description:
  // Overridden to create output data of appropriate type.
  virtual int ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*);
protected:
  vtkPVGlyphFilter();
  ~vtkPVGlyphFilter();

  // Standard Pipeline methods
  virtual int RequestData(
      vtkInformation*, vtkInformationVector**,vtkInformationVector*);
  virtual int RequestDataObject(
      vtkInformation*, vtkInformationVector**,vtkInformationVector*);
  virtual int FillInputPortInformation(int, vtkInformation*);
  virtual int FillOutputPortInformation(int, vtkInformation*);

  // Description:
  // Returns 1 if point is to be glyped, otherwise returns 0.
  virtual int IsPointVisible(vtkDataSet* ds, vtkIdType ptId);

  int GlyphMode;
  int MaximumNumberOfSamplePoints;
  int Seed;
  int Stride;
  vtkMultiProcessController* Controller;

private:
  vtkPVGlyphFilter(const vtkPVGlyphFilter&);  // Not implemented.
  void operator=(const vtkPVGlyphFilter&);  // Not implemented.

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
