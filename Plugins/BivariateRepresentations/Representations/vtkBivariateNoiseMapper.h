// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkBivariateNoiseMapper
 * @brief Mapper to visualize bivariate data with noise
 *
 * The vtkBivariateNoiseMapper allow to visualize bivariate data with Perlin noise.
 * A second array can be passed to this mapper. The values of this array will be used
 * to control the noise intensity: the bigger the values are, the bigger the intensity
 * of the noise is (values are clamped between 0 and 1; a value of 0 corresponds to no
 * noise at all.)
 *
 * The user can control the noise with 4 parameters:
 * - the Frequency parameter controls the number of cells (tiles) used to
 *   generate the noise,
 * - the Amplitude parameter controls how much the values of the noise array
 *   affect the amplitude of the noise,
 * - the Speed parameter controls the speed of the noise animation,
 * - the NbOctaves controls the number of layers of noise (octaves) to generate.
 *   Please note that adding octaves leads to a performance cost.
 *
 * Please note that the vtkBivariateNoiseMapper only works with point data.
 * Furthermore:
 * - you must always define a 1st array for coloring,
 * - the mapper only accept scalar values for the noise array,
 * - the InterpolateScalarsBeforeMapping option should be set to true.
 * If these requirement are not fullfiled, the vtkBivariateNoiseMapper
 * acts as the standard vtkOpenGLPolyDataMapper.
 *
 * This mapper shader implementation is based on concepts and ideas
 * comming from the following sources:
 * - https://en.wikipedia.org/wiki/Perlin_noise
 * - https://thebookofshaders.com/11/
 * - https://thebookofshaders.com/13/
 * - https://x-engineer.org/bilinear-interpolation/
 *
 * @sa vtkOpenGLPolyDataMapper vtkBivariateNoiseRepresentation
 */

#ifndef vtkBivariateNoiseMapper_h
#define vtkBivariateNoiseMapper_h

#include "vtkCompositePolyDataMapper.h"

#include "vtkBivariateRepresentationsModule.h" // for export macro

#include <memory> // for unique_ptr

class VTKBIVARIATEREPRESENTATIONS_EXPORT vtkBivariateNoiseMapper : public vtkCompositePolyDataMapper
{
public:
  static vtkBivariateNoiseMapper* New();
  vtkTypeMacro(vtkBivariateNoiseMapper, vtkCompositePolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the frequency of the noise.
   * The frequency correspond to the "number of subdivisions" of the 4D
   * grid from which the first layer of noise is generated (can be a floating value).
   * Default is 30.0.
   */
  void SetFrequency(double frequency);
  double GetFrequency() const;

  /**
   * Set the final amplitude of the noise.
   * This acts as a multiplier controlling the impact of the noise
   * value on the color of the first data array.
   * Default is 1.0.
   */
  void SetAmplitude(double amplitude);
  double GetAmplitude() const;

  /**
   * Set the speed of the nois animation.
   * This is a multiplier applied to the original speed.
   * Default is 1.0.
   */
  void SetSpeed(double speed);
  double GetSpeed() const;

  /**
   * Set the number of layers of noise.
   * This controls the number of octaves (layers) of noise added to the resulting noise.
   * For each extra layer, the noise is recomputed using a frequency multiplied by two,
   * and an amplitude divided by 2 comparatively to the previous iteration.
   * Implies a performance cost (noise is recomputed for each layer).
   * Default is 3.0.
   */
  void SetNbOfOctaves(int nbOctaves);
  int GetNbOfOctaves() const;

  /**
   * Initialize the mapper if needed then render.
   */
  void Render(vtkRenderer* ren, vtkActor* act) override;

  long GetStartTime() const;

protected:
  vtkBivariateNoiseMapper();
  ~vtkBivariateNoiseMapper() override;

  /**
   * Starts the internal timer for noise animation.
   * Done automatically in the first Render() call.
   */
  void Initialize();

  vtkCompositePolyDataMapperDelegator* CreateADelegator() override;

private:
  vtkBivariateNoiseMapper(const vtkBivariateNoiseMapper&) = delete;
  void operator=(const vtkBivariateNoiseMapper&) = delete;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif

// VTK-HeaderTest-Exclude: vtkBivariateNoiseMapper.h
