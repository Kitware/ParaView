/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVX3DExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen, Kristian Sons
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVX3DExporter.h"

#include "vtkActor2DCollection.h"
#include "vtkContext2DScalarBarActor.h"
#include "vtkCoordinate.h"
#include "vtkImageClip.h"
#include "vtkImageTransparencyFilter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRect.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindowToImageFilter.h"
#include "vtkX3D.h"
#include "vtkX3DExporterWriter.h"

#include <algorithm>
#include <set>
#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVX3DExporter);

//----------------------------------------------------------------------------
vtkPVX3DExporter::vtkPVX3DExporter()
  : ExportColorLegends(false)
{
}

//----------------------------------------------------------------------------
vtkPVX3DExporter::~vtkPVX3DExporter()
{
}

//----------------------------------------------------------------------------
void vtkPVX3DExporter::WriteAdditionalNodes(vtkX3DExporterWriter* writer)
{
  if (this->ExportColorLegends)
  {
    this->WriteColorLegends(writer);
  }
}

//----------------------------------------------------------------------------
void vtkPVX3DExporter::WriteColorLegends(vtkX3DExporterWriter* writer)
{
  vtkRenderer* bottomRenderer = nullptr;
  vtkRenderer* annotationRenderer = nullptr;

  vtkRendererCollection* renderers = this->RenderWindow->GetRenderers();
  renderers->InitTraversal();
  while (vtkRenderer* currentRenderer = renderers->GetNextItem())
  {
    if (currentRenderer->GetLayer() == 0)
    {
      // We need the bottom renderer to clear the render window between
      // screen shots of the individual color maps.
      bottomRenderer = currentRenderer;
    }
    else if (currentRenderer->GetLayer() == 2)
    {
      // Hard-coded layer for ParaView
      annotationRenderer = currentRenderer;
    }

    if (currentRenderer != annotationRenderer)
    {
      // Turn off non-annotation renderers temporarily while saving color legends
      currentRenderer->DrawOff();
    }
  }

  if (!annotationRenderer)
  {
    vtkErrorMacro("Could not find annotation renderer. Color legends will not be saved.");
    return;
  }

  typedef std::set<vtkContext2DScalarBarActor*> VisibleActorType;
  VisibleActorType visibleActors;

  vtkActor2DCollection* actors2D = annotationRenderer->GetActors2D();
  actors2D->InitTraversal();
  while (vtkActor2D* currentActor = actors2D->GetNextItem())
  {
    if (currentActor->IsA("vtkContext2DScalarBarActor"))
    {
      vtkContext2DScalarBarActor* sbActor = vtkContext2DScalarBarActor::SafeDownCast(currentActor);
      if (sbActor->GetVisibility() == 1)
      {
        visibleActors.insert(sbActor);
      }

      // Turn visibility off. Each actor will be turned back on when it is time
      // to save a screenshot of it, and then re-enabled in the cleanup phase.
      sbActor->VisibilityOff();
    }
  }

  for (VisibleActorType::iterator i = visibleActors.begin(); i != visibleActors.end(); ++i)
  {
    vtkContext2DScalarBarActor* sbActor = *i;

    // Turn just this scalar bar actor on.
    sbActor->VisibilityOn();
    this->WriteColorLegend(bottomRenderer, annotationRenderer, sbActor, writer);
    sbActor->VisibilityOff();
  }

  // Make all previously visible actors visible again.
  for (VisibleActorType::iterator i = visibleActors.begin(); i != visibleActors.end(); ++i)
  {
    vtkContext2DScalarBarActor* sbActor = *i;
    sbActor->VisibilityOn();
  }

  // Enable all renderers once again.
  renderers->InitTraversal();
  while (vtkRenderer* currentRenderer = renderers->GetNextItem())
  {
    currentRenderer->DrawOn();
  }

  // Issue a render to update the view
  this->RenderWindow->Render();
}

namespace
{
typedef struct RendererState_struct
{
  bool GradientBackground;
  double Background[3];
  double Background2[3];
  bool TexturedBackground;
  vtkTexture* Texture;
} RendererState;

//----------------------------------------------------------------------------
RendererState SaveRendererState(vtkRenderer* renderer)
{
  RendererState state;
  state.GradientBackground = renderer->GetGradientBackground();
  renderer->GetBackground(state.Background);
  renderer->GetBackground2(state.Background2);
  state.TexturedBackground = renderer->GetTexturedBackground();
  state.Texture = renderer->GetBackgroundTexture();

  return state;
}

//----------------------------------------------------------------------------
void RestoreRendererState(vtkRenderer* renderer, RendererState& state)
{
  renderer->SetGradientBackground(state.GradientBackground);
  renderer->SetBackground(state.Background);
  renderer->SetBackground2(state.Background2);
  renderer->SetTexturedBackground(state.TexturedBackground);
  if (state.TexturedBackground)
  {
    renderer->SetBackgroundTexture(state.Texture);
  }
}

//----------------------------------------------------------------------------
vtkImageData* CaptureScalarBarImage(vtkRenderer* bottomRenderer, vtkRenderer* annotationRenderer,
  vtkContext2DScalarBarActor* actor, double r, double g, double b)
{
  vtkCoordinate* coordinate = actor->GetPositionCoordinate();
  double* position = coordinate->GetComputedDoubleDisplayValue(annotationRenderer);

  // Need to clear the bottom renderer to get the desired background color.
  bottomRenderer->SetBackground(r, g, b);
  bottomRenderer->DrawOn();
  bottomRenderer->Clear();
  bottomRenderer->DrawOff();

  vtkNew<vtkWindowToImageFilter> windowToImageFilter;
  windowToImageFilter->SetInput(annotationRenderer->GetRenderWindow());
  windowToImageFilter->Update();

  // Crop the screenshot to the bounding rect of the color legend.
  vtkRectf boundingRect = actor->GetBoundingRect();
  int bounds[6];
  bounds[0] = static_cast<int>(boundingRect.GetX() + position[0]);
  bounds[1] = bounds[0] + static_cast<int>(boundingRect.GetWidth());
  bounds[2] = static_cast<int>(boundingRect.GetY() + position[1]);
  bounds[3] = bounds[2] + static_cast<int>(boundingRect.GetHeight());
  bounds[4] = 0;
  bounds[5] = 0;

  vtkNew<vtkImageClip> clipper;
  clipper->SetInputConnection(windowToImageFilter->GetOutputPort());
  clipper->SetOutputWholeExtent(bounds);
  clipper->ClipDataOn();
  clipper->Update();

  vtkImageData* image = vtkImageData::New();
  image->ShallowCopy(clipper->GetOutput());

  return image;
}
}

//----------------------------------------------------------------------------
void vtkPVX3DExporter::WriteColorLegend(vtkRenderer* bottomRenderer,
  vtkRenderer* annotationRenderer, vtkContext2DScalarBarActor* actor, vtkX3DExporterWriter* writer)
{
  // Save renderer state
  RendererState rendererState = SaveRendererState(bottomRenderer);

  // Set up renderer background to be a single color
  bottomRenderer->SetGradientBackground(false);
  bottomRenderer->SetTexturedBackground(false);

  // Render white-background image
  vtkSmartPointer<vtkImageData> whiteImage;
  whiteImage.TakeReference(
    CaptureScalarBarImage(bottomRenderer, annotationRenderer, actor, 1, 1, 1));

  // Do the same for the black-background image
  vtkSmartPointer<vtkImageData> blackImage;
  blackImage.TakeReference(
    CaptureScalarBarImage(bottomRenderer, annotationRenderer, actor, 0, 0, 0));

  vtkNew<vtkImageTransparencyFilter> transparency;
  transparency->SetInputData(whiteImage);
  transparency->AddInputData(blackImage);
  transparency->Update();
  vtkImageData* image = transparency->GetOutput();
  vtkPointData* pd = image->GetPointData();
  vtkUnsignedCharArray* pixelData = vtkUnsignedCharArray::SafeDownCast(pd->GetScalars());
  if (!pixelData)
  {
    vtkErrorMacro("Pixel data not found or was not of type vtkUnsignedCharArray");
    return;
  }

  std::stringstream colorLegendNameSS;
  colorLegendNameSS << actor->GetTitle();
  if (actor->GetComponentTitle() && actor->GetComponentTitle()[0] != '\0')
  {
    colorLegendNameSS << "-" << actor->GetComponentTitle();
  }

  // Replace spaces with underscores
  std::string colorLegendName(colorLegendNameSS.str());
  std::replace(colorLegendName.begin(), colorLegendName.end(), ' ', '_');

  writer->StartNode(vtkX3D::PixelTexture);
  writer->SetField(vtkX3D::DEF, colorLegendName.c_str());
  std::vector<int> sfImageValues(3 + pixelData->GetNumberOfValues(), 0);
  sfImageValues[0] = image->GetDimensions()[0];
  sfImageValues[1] = image->GetDimensions()[1];
  sfImageValues[2] = pixelData->GetNumberOfComponents();

  for (vtkIdType i = 3; i < pixelData->GetNumberOfValues() + 3; ++i)
  {
    unsigned char value = pixelData->GetValue(i);
    sfImageValues[i] = static_cast<int>(value);
  }

  writer->SetField(vtkX3D::SFIMAGE, &sfImageValues[0], sfImageValues.size(), true);
  writer->EndNode();

  // Restore renderer state
  RestoreRendererState(bottomRenderer, rendererState);
}

//----------------------------------------------------------------------------
void vtkPVX3DExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
