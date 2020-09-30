/*=========================================================================

  Program:   ParaView
  Module:    vtkParticlePipeline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkParticlePipeline.h"

#include "vtkActor.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataSetMapper.h"
#include "vtkExecutive.h"
#include "vtkGlyph3DMapper.h"
#include "vtkIceTCompositePass.h"
#include "vtkIceTSynchronizedRenderers.h"
#include "vtkInformation.h"
#include "vtkLightKit.h"
#include "vtkLightsPass.h"
#include "vtkMPIController.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOutlineSource.h"
#include "vtkPNGWriter.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSynchronizedRenderWindows.h"
#include "vtkTrivialProducer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkWindowToImageFilter.h"
#include "vtkXMLPUnstructuredGridReader.h"
#include "vtkXMLPUnstructuredGridWriter.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkParticlePipeline);

vtkParticlePipeline::vtkParticlePipeline()
{
  this->Filename = 0;

  this->ParticleRadius = 1.0;

  this->CameraThetaAngle = 0.0;
  this->CameraPhiAngle = 0.0;
  this->CameraDistance = 0.0;

  for (int i = 0; i < 3; i++)
  {
    this->Bounds[2 * i] = -1.0;
    this->Bounds[2 * i + 1] = 1.0;
  }

  this->AttributeMaximum = 1.0;
  this->AttributeMinimum = 0.0;

  this->input = vtkTrivialProducer::New();
  this->lut = vtkColorTransferFunction::New();
  this->sphere = vtkSphereSource::New();
  this->outline = vtkOutlineSource::New();
  this->particleMapper = vtkGlyph3DMapper::New();
  this->outlineMapper = vtkPolyDataMapper::New();
  this->particleActor = vtkActor::New();
  this->outlineActor = vtkActor::New();
  this->lightKit = vtkLightKit::New();
  this->syncRen = vtkIceTSynchronizedRenderers::New();
  this->syncWin = vtkSynchronizedRenderWindows::New();
  this->window = vtkRenderWindow::New();
  this->renderer = vtkRenderer::New();
  this->w2i = vtkWindowToImageFilter::New();
  this->writer = vtkPNGWriter::New();

  this->SetupPipeline();
}

vtkParticlePipeline::~vtkParticlePipeline()
{
  this->input->Delete();
  this->lut->Delete();
  this->sphere->Delete();
  this->outline->Delete();
  this->particleMapper->Delete();
  this->outlineMapper->Delete();
  this->particleActor->Delete();
  this->outlineActor->Delete();
  this->lightKit->Delete();
  this->syncRen->Delete();
  this->syncWin->Delete();
  this->window->Delete();
  this->renderer->Delete();
  this->w2i->Delete();
  this->writer->Delete();
}

void vtkParticlePipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Filename: " << this->Filename << endl;
  os << indent << "ParticleRadius: " << this->ParticleRadius << endl;
  os << indent << "CameraThetaAngle: " << this->CameraThetaAngle << endl;
  os << indent << "CameraPhiAngle: " << this->CameraPhiAngle << endl;
  os << indent << "CameraDistance: " << this->CameraDistance << endl;
  os << indent << "AttributeMaximum: " << this->AttributeMaximum << endl;
  os << indent << "AttributeMinimum: " << this->AttributeMinimum << endl;
}

int vtkParticlePipeline::RequestDataDescription(vtkCPDataDescription* desc)
{
  if (!desc)
  {
    vtkWarningMacro("data description is NULL");
    return 0;
  }
  return 1;
}

void vtkParticlePipeline::SetupPipeline()
{
  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();

  this->lut->SetColorSpaceToDiverging();

  this->particleMapper->SetInputConnection(input->GetOutputPort());
  this->particleMapper->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Attribute");
  this->particleMapper->SetSourceConnection(sphere->GetOutputPort());
  this->particleMapper->SetLookupTable(lut);
  this->particleMapper->ScalarVisibilityOn();
  this->particleMapper->SetColorModeToMapScalars();
  this->particleMapper->SetScalarModeToUsePointFieldData();
  this->particleMapper->SelectColorArray("Attribute");
  this->particleMapper->SetScaling(false);
  this->particleMapper->SetOrient(false);

  this->particleActor->SetMapper(particleMapper);

  this->outlineMapper->SetInputConnection(this->outline->GetOutputPort());
  this->outlineActor->SetMapper(this->outlineMapper);
  this->outlineActor->GetProperty()->SetColor(0.7, 0.7, 0.7);
  this->outlineActor->GetProperty()->SetAmbient(1.0);
  this->outlineActor->GetProperty()->SetDiffuse(0.0);

  this->renderer->SetBackground(0.0, 0.0, 0.0);
  this->renderer->AddActor(particleActor);
  this->renderer->AddActor(outlineActor);

  this->window->AddRenderer(renderer);
  this->window->SetPosition(ctrl->GetLocalProcessId() * (256 + 10), 0);
  this->window->SetSize(256, 256);
  this->window->DoubleBufferOn();
  this->window->SwapBuffersOff();

  this->lightKit->AddLightsToRenderer(renderer);

  VTK_CREATE(vtkRenderPassCollection, passes);
  VTK_CREATE(vtkLightsPass, lights);
  passes->AddItem(lights);
  VTK_CREATE(vtkOpaquePass, opaque);
  passes->AddItem(opaque);

  VTK_CREATE(vtkSequencePass, seq);
  seq->SetPasses(passes);

  VTK_CREATE(vtkIceTCompositePass, iceTPass);
  iceTPass->SetController(ctrl);
  iceTPass->SetRenderPass(seq);
  iceTPass->SetDataReplicatedOnAllProcesses(false);

  VTK_CREATE(vtkCameraPass, cameraP);
  cameraP->SetDelegatePass(iceTPass);
  vtkOpenGLRenderer* glRenderer = vtkOpenGLRenderer::SafeDownCast(this->renderer);
  if (glRenderer != NULL)
  {
    glRenderer->SetPass(cameraP);
  }
  else
  {
    vtkErrorMacro("Cannot cast renderer to vtkOpenGLRenderer!");
    return;
  }

  this->syncWin->SetRenderWindow(window);
  this->syncWin->SetParallelController(ctrl);
  this->syncWin->SetIdentifier(235827347);

  this->syncRen->SetRenderer(renderer);
  this->syncRen->SetParallelController(ctrl);
  this->syncRen->SetParallelRendering(true);
  this->syncRen->WriteBackImagesOn();
  this->syncRen->SetRootProcessId(0);

  this->w2i->SetInput(window);
  // w2i->SetScale(magnification);
  this->w2i->ReadFrontBufferOff();
  this->w2i->ShouldRerenderOff();
  this->w2i->FixBoundaryOn();

  this->writer->SetInputConnection(w2i->GetOutputPort());
}

int vtkParticlePipeline::CoProcess(vtkCPDataDescription* desc)
{
  int timestep = desc->GetTimeStep();

  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();

  this->input->SetOutput(desc->GetInputDescriptionByName("input")->GetGrid());

  this->lut->AddRGBPoint(this->AttributeMinimum, 0.23, 0.3, 0.754);
  this->lut->AddRGBPoint(this->AttributeMaximum, 0.706, 0.016, 0.15);

  this->sphere->SetRadius(this->ParticleRadius);
  this->sphere->Update();
  this->particleMapper->Update();

  this->outline->SetBounds(this->Bounds);
  this->outline->Update();
  this->outlineMapper->Update();

  vtkCamera* cam = this->renderer->GetActiveCamera();
  cam->SetFocalPoint(0, 0, 0);
  cam->SetViewUp(0, 1, 0);
  cam->SetPosition(0, 0, 1);
  cam->Azimuth(vtkMath::DegreesFromRadians(this->CameraThetaAngle));
  cam->Elevation(vtkMath::DegreesFromRadians(this->CameraPhiAngle));
  if (this->CameraDistance == 0.0)
  {
    this->renderer->ResetCamera(this->Bounds);
  }
  else
  {
    cam->Dolly(1.0 / this->CameraDistance);
  }

  if (ctrl->GetLocalProcessId() == 0)
  {
    this->window->Render();
    ctrl->TriggerBreakRMIs();
    ctrl->Barrier();
  }
  else
  {
    ctrl->ProcessRMIs();
    ctrl->Barrier();
  }

  if (ctrl->GetLocalProcessId() == 0)
  {
    this->w2i->Modified();
    char* outstring = new char[strlen(this->Filename) + 32];
    sprintf(outstring, this->Filename, timestep);
    this->writer->SetFileName(outstring);
    this->writer->Write();
    delete[] outstring;
  }

  return 1;
}

#undef VTK_CREATE
