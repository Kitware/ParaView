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
#include "vtkStreamLinesModule.h" // for export macro

class vtkActor;
class vtkDataSet;
class vtkImageData;
class vtkRenderer;

class VTKSTREAMLINES_EXPORT vtkStreamLinesMapper : public vtkMapper
{
public:
  static vtkStreamLinesMapper* New();
  vtkTypeMacro(vtkStreamLinesMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  //@{
  /**
   * Some introspection on the type of data the mapper will render
   * used by props to determine if they should invoke the mapper
   * on a specific rendering pass.
   */
  bool HasOpaqueGeometry() override { return true; }
  bool HasTranslucentPolygonalGeometry() override { return false; }
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  void Render(vtkRenderer* ren, vtkActor* vol) override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkStreamLinesMapper();
  ~vtkStreamLinesMapper() override;

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
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkStreamLinesMapper(const vtkStreamLinesMapper&) = delete;
  void operator=(const vtkStreamLinesMapper&) = delete;
};

#endif
