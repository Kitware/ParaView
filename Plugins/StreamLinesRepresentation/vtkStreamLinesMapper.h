/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamLinesMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStreamLinesMapper
 * @brief   Mapper for live stream lines rendering of vtkDataSet.
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux and Bastien Jacquet, Kitware 2017
 * This work was supported by Total SA.
 *
 * @sa
 * vtkVolumeMapper vtkUnstructuredGridVolumeMapper
 */

#ifndef vtkStreamLinesMapper_h
#define vtkStreamLinesMapper_h

#include "vtkMapper.h"

class vtkActor;
class vtkDataSet;
class vtkImageData;
class vtkRenderer;

class VTK_EXPORT vtkStreamLinesMapper : public vtkMapper
{
public:
  static vtkStreamLinesMapper* New();
  vtkTypeMacro(vtkStreamLinesMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set animation status. Default is true.
   */
  virtual void SetAnimate(bool);
  vtkGetMacro(Animate, bool);
  vtkBooleanMacro(Animate, bool);
  //@}

  //@{
  /**
   * Get/Set the Alpha blending between new trajectory and previous.
   * Default is 0.95
   */
  vtkSetMacro(Alpha, double);
  vtkGetMacro(Alpha, double);
  //@}

  //@{
  /**
   * Get/Set the integration step factor.
   * Default is 0.01
   */
  vtkSetMacro(StepLength, double);
  vtkGetMacro(StepLength, double);
  //@}

  //@{
  /**
   * Get/Set the number of particles.
   * Default is 1000.
   */
  void SetNumberOfParticles(int);
  vtkGetMacro(NumberOfParticles, int);
  //@}

  //@{
  /**
   * Get/Set the maximum number of iteration before particles die.
   * Default is 600.
   */
  vtkSetMacro(MaxTimeToLive, int);
  vtkGetMacro(MaxTimeToLive, int);
  //@}

  //@{
  /**
   * Get/Set the maximum number of animation steps before the animation stops.
   * Default is 1.
   */
  vtkSetMacro(NumberOfAnimationSteps, int);
  vtkGetMacro(NumberOfAnimationSteps, int);
  //@}

  /**
   * Returns if the mapper does not expect to have translucent geometry. This
   * may happen when using ColorMode is set to not map scalars i.e. render the
   * scalar array directly as colors and the scalar array has opacity i.e. alpha
   * component.  Default implementation simply returns true. Note that even if
   * this method returns true, an actor may treat the geometry as translucent
   * since a constant translucency is set on the property, for example.
   */
  bool GetIsOpaque() VTK_OVERRIDE { return true; }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  void Render(vtkRenderer* ren, vtkActor* vol) VTK_OVERRIDE;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) VTK_OVERRIDE;

protected:
  vtkStreamLinesMapper();
  ~vtkStreamLinesMapper();

  double Alpha;
  double StepLength;
  int MaxTimeToLive;
  int NumberOfParticles;
  int NumberOfAnimationSteps;
  int AnimationSteps;
  bool Animate;
  class Private;
  Private* Internal;

  friend class Private;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  vtkStreamLinesMapper(const vtkStreamLinesMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStreamLinesMapper&) VTK_DELETE_FUNCTION;
};

#endif
