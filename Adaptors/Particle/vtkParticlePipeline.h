/*=========================================================================

  Program:   ParaView
  Module:    vtkParticlePipeline.h:

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkParticlePipeline_H
#define vtkParticlePipeline_H

#include "vtkCPPipeline.h"
#include "vtkPVAdaptorsParticleModule.h" // For export macros

class vtkTrivialProducer;
class vtkColorTransferFunction;
class vtkSphereSource;
class vtkOutlineSource;
class vtkGlyph3DMapper;
class vtkPolyDataMapper;
class vtkActor;
class vtkLightKit;
class vtkIceTSynchronizedRenderers;
class vtkSynchronizedRenderWindows;
class vtkRenderWindow;
class vtkRenderer;
class vtkWindowToImageFilter;
class vtkPNGWriter;

class VTKPVADAPTORSPARTICLE_EXPORT vtkParticlePipeline : public vtkCPPipeline
{
public:
  static vtkParticlePipeline* New();
  vtkTypeMacro(vtkParticlePipeline, vtkCPPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual int RequestDataDescription(vtkCPDataDescription* desc);

  virtual int CoProcess(vtkCPDataDescription* desc);

  // Description:
  // name of the image file to output
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  // Description:
  // radius of the sphere glyph to place on the particles
  vtkSetMacro(ParticleRadius, double);
  vtkGetMacro(ParticleRadius, double);

  // Description:
  // angle of the camera around the mass of particles in the x-z plane
  vtkSetMacro(CameraThetaAngle, double);
  vtkGetMacro(CameraThetaAngle, double);

  // Description:
  // angle the camera makes with the y axis (relative to particles' center)
  vtkSetMacro(CameraPhiAngle, double);
  vtkGetMacro(CameraPhiAngle, double);

  // Description:
  // distance the camera is from the particles' center
  // zero means to find the optimal viewing distance (default: zero)
  vtkSetMacro(CameraDistance, double);
  vtkGetMacro(CameraDistance, double);

  // Description:
  // bounds of the particle space.  This will be used to set the camera
  // distance (if requested) and the box outline.
  vtkSetVector6Macro(Bounds, double);
  vtkGetVectorMacro(Bounds, double, 6);

  // Description:
  // max and min value for the attributes to use in defining the color lookup
  vtkSetMacro(AttributeMaximum, double);
  vtkGetMacro(AttributeMaximum, double);
  vtkSetMacro(AttributeMinimum, double);
  vtkGetMacro(AttributeMinimum, double);

protected:
  vtkParticlePipeline();
  virtual ~vtkParticlePipeline();

  void SetupPipeline();

  char* Filename;

  double ParticleRadius;

  double CameraThetaAngle;
  double CameraPhiAngle;
  double CameraDistance;

  double Bounds[6];

  double AttributeMaximum;
  double AttributeMinimum;

  vtkTrivialProducer* input;
  vtkColorTransferFunction* lut;
  vtkSphereSource* sphere;
  vtkOutlineSource* outline;
  vtkGlyph3DMapper* particleMapper;
  vtkPolyDataMapper* outlineMapper;
  vtkActor* particleActor;
  vtkActor* outlineActor;
  vtkLightKit* lightKit;
  vtkIceTSynchronizedRenderers* syncRen;
  vtkSynchronizedRenderWindows* syncWin;
  vtkRenderWindow* window;
  vtkRenderer* renderer;
  vtkWindowToImageFilter* w2i;
  vtkPNGWriter* writer;

private:
  vtkParticlePipeline(const vtkParticlePipeline&) = delete;
  void operator=(const vtkParticlePipeline&) = delete;
};

#endif /* vtkParticlePipeline_H */
