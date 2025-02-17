// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVLODActor.h"

#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"

#include <algorithm>
#include <cmath>

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayActorNode.h"
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODActor);

vtkCxxSetObjectMacro(vtkPVLODActor, LODMapper, vtkMapper);
//----------------------------------------------------------------------------
vtkPVLODActor::vtkPVLODActor()
{
  vtkMatrix4x4* m;

  // get a hardware dependent actor and mappers
  this->Device = vtkActor::New();
  m = vtkMatrix4x4::New();
  this->Device->SetUserMatrix(m);
  m->Delete();

  this->LODMapper = nullptr;

  this->EnableLOD = 0;
}

//----------------------------------------------------------------------------
vtkPVLODActor::~vtkPVLODActor()
{
  this->SetLODMapper(nullptr);
  this->Device->Delete();
  this->Device = nullptr;
}

//----------------------------------------------------------------------------
// We use points as the size of the data, because cells cqan mislead.
// A good example is verts.  One cell can contain any number of vertices.
vtkMapper* vtkPVLODActor::SelectMapper()
{
  if (this->Mapper == nullptr || this->Mapper->GetInputDataObject(0, 0) == nullptr)
  {
    return this->LODMapper;
  }
  if (this->LODMapper == nullptr || this->LODMapper->GetInputDataObject(0, 0) == nullptr)
  {
    return this->Mapper;
  }

  if (this->EnableLOD)
  {
    return this->LODMapper;
  }

  return this->Mapper;
}

//----------------------------------------------------------------------------
void vtkPVLODActor::Render(vtkRenderer* ren, vtkMapper* vtkNotUsed(m))
{
  vtkMatrix4x4* matrix;
  vtkMapper* mapper;

  if (this->Mapper == nullptr)
  {
    vtkErrorMacro("No mapper for actor.");
    return;
  }

  mapper = this->SelectMapper();

  if (mapper == nullptr)
  {
    return;
  }

  /* render the property */
  if (!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }
  this->Property->Render(this, ren);
  if (this->BackfaceProperty)
  {
    this->BackfaceProperty->BackfaceRender(this, ren);
    this->Device->SetBackfaceProperty(this->BackfaceProperty);
  }
  this->Device->SetProperty(this->Property);
  this->Device->SetForceOpaque(this->GetForceOpaque());
  this->Device->SetForceTranslucent(this->GetForceTranslucent());
  bool inTrans = this->IsRenderingTranslucentPolygonalGeometry();
  this->Device->SetIsRenderingTranslucentPolygonalGeometry(inTrans);

  /* render the texture */
  if (this->Texture)
  {
    this->Texture->Render(ren);
  }
  this->Device->SetTexture(this->Texture);
  this->Device->SetShaderProperty(this->ShaderProperty);

  // make sure the device has the same matrix
  matrix = this->Device->GetUserMatrix();
  this->GetMatrix(matrix);

  // Store information on time it takes to render.
  // We might want to estimate time from the number of polygons in mapper.
  vtkInformation* info = this->GetPropertyKeys();
  this->Device->SetPropertyKeys(info);
  this->Device->SetMapper(mapper);
  this->Device->Render(ren, mapper);
  if (this->Texture)
  {
    this->Texture->PostRender(ren);
  }
  this->Property->PostRender(this, ren);
  this->Device->SetIsRenderingTranslucentPolygonalGeometry(false);
  this->EstimatedRenderTime = mapper->GetTimeToDraw();
}

int vtkPVLODActor::RenderOpaqueGeometry(vtkViewport* vp)
{
  int renderedSomething = 0;
  vtkRenderer* ren = static_cast<vtkRenderer*>(vp);

  if (!this->Mapper)
  {
    return 0;
  }

  // make sure we have a property
  if (!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }

  // is this actor opaque ?
  // Do this check only when not in selection mode
  if (this->HasOpaqueGeometry() || (ren->GetSelector() && this->Property->GetOpacity() > 0.0))
  {
    this->Render(ren, this->Mapper);
    renderedSomething = 1;
  }

  return renderedSomething;
}

void vtkPVLODActor::ReleaseGraphicsResources(vtkWindow* renWin)
{
  vtkActor::ReleaseGraphicsResources(renWin);

  // broadcast the message down to the individual LOD mappers
  if (this->LODMapper)
  {
    this->LODMapper->ReleaseGraphicsResources(renWin);
  }
}

// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* vtkPVLODActor::GetBounds()
{
  int i, n;
  double *bounds, bbox[24], *fptr;
  vtkMapper* mapper = this->GetMapper();

  vtkDebugMacro(<< "Getting Bounds");

  // get the bounds of the Mapper if we have one
  if (!mapper)
  {
    return this->Bounds;
  }

  bounds = mapper->GetBounds();
  // Check for the special case when the mapper's bounds are unknown
  if (!bounds)
  {
    return bounds;
  }

  // Check for the special case when the actor is empty.
  if (bounds[0] > bounds[1])
  {
    memcpy(this->MapperBounds, bounds, 6 * sizeof(double));
    vtkMath::UninitializeBounds(this->Bounds);
    this->BoundsMTime.Modified();
    return this->Bounds;
  }

  // Check if we have cached values for these bounds - we cache the
  // values returned by this->Mapper->GetBounds() and we store the time
  // of caching. If the values returned this time are different, or
  // the modified time of this class is newer than the cached time,
  // then we need to rebuild. Also, if this prop3d is not in world
  // coordinates, then its location may be affected by the renderwindow
  // physical to world matrix or other device to world matrix, and the
  // bounds need to be recomputed in this case as well.
  // NOLINTNEXTLINE(bugprone-suspicious-memory-comparison)
  if ((memcmp(this->MapperBounds, bounds, 6 * sizeof(double)) != 0) ||
    (this->GetMTime() > this->BoundsMTime) || (this->CoordinateSystem != vtkProp3D::WORLD))
  {
    vtkDebugMacro(<< "Recomputing bounds...");

    memcpy(this->MapperBounds, bounds, 6 * sizeof(double));

    // fill out vertices of a bounding box
    bbox[0] = bounds[1];
    bbox[1] = bounds[3];
    bbox[2] = bounds[5];
    bbox[3] = bounds[1];
    bbox[4] = bounds[2];
    bbox[5] = bounds[5];
    bbox[6] = bounds[0];
    bbox[7] = bounds[2];
    bbox[8] = bounds[5];
    bbox[9] = bounds[0];
    bbox[10] = bounds[3];
    bbox[11] = bounds[5];
    bbox[12] = bounds[1];
    bbox[13] = bounds[3];
    bbox[14] = bounds[4];
    bbox[15] = bounds[1];
    bbox[16] = bounds[2];
    bbox[17] = bounds[4];
    bbox[18] = bounds[0];
    bbox[19] = bounds[2];
    bbox[20] = bounds[4];
    bbox[21] = bounds[0];
    bbox[22] = bounds[3];
    bbox[23] = bounds[4];

    // save the old transform
    this->Transform->Push();
    this->Transform->SetMatrix(this->GetMatrix());

    // and transform into actors coordinates
    fptr = bbox;
    for (n = 0; n < 8; n++)
    {
      this->Transform->TransformPoint(fptr, fptr);
      fptr += 3;
    }

    this->Transform->Pop();

    // now calc the new bounds
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
    for (i = 0; i < 8; i++)
    {
      for (n = 0; n < 3; n++)
      {
        this->Bounds[n * 2] = std::min(this->Bounds[n * 2], bbox[i * 3 + n]);
        this->Bounds[n * 2 + 1] = std::max(this->Bounds[n * 2 + 1], bbox[i * 3 + n]);
      }
    }
    this->BoundsMTime.Modified();
  }

  return this->Bounds;
}

//----------------------------------------------------------------------------
void vtkPVLODActor::Modified()
{
  this->Device->Modified();
  this->vtkActor::Modified();
}

void vtkPVLODActor::ShallowCopy(vtkProp* prop)
{
  vtkPVLODActor* a = vtkPVLODActor::SafeDownCast(prop);
  if (a != nullptr)
  {
    this->SetLODMapper(a->GetLODMapper());
  }

  // Now do superclass
  this->vtkActor::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkPVLODActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->LODMapper)
  {
    os << indent << "LODMapper: " << this->GetLODMapper() << endl;
  }

  os << indent << "EnableLOD: " << this->EnableLOD << endl;
}

//----------------------------------------------------------------------------
void vtkPVLODActor::SetEnableScaling(int val)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  if (this->Mapper)
  {
    vtkInformation* info = this->Mapper->GetInformation();
    info->Set(vtkOSPRayActorNode::ENABLE_SCALING(), val);
  }
  if (this->LODMapper)
  {
    vtkInformation* info = this->LODMapper->GetInformation();
    info->Set(vtkOSPRayActorNode::ENABLE_SCALING(), val);
  }
#else
  (void)val;
#endif
}

//----------------------------------------------------------------------------
void vtkPVLODActor::SetScalingArrayName(const char* val)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  if (this->Mapper)
  {
    vtkInformation* mapperInfo = this->Mapper->GetInformation();
    mapperInfo->Set(vtkOSPRayActorNode::SCALE_ARRAY_NAME(), val);
  }
  if (this->LODMapper)
  {
    vtkInformation* mapperInfo = this->LODMapper->GetInformation();
    mapperInfo->Set(vtkOSPRayActorNode::SCALE_ARRAY_NAME(), val);
  }
#else
  (void)val;
#endif
}

//----------------------------------------------------------------------------
void vtkPVLODActor::SetScalingFunction(vtkPiecewiseFunction* pwf)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  if (this->Mapper)
  {
    vtkInformation* mapperInfo = this->Mapper->GetInformation();
    mapperInfo->Set(vtkOSPRayActorNode::SCALE_FUNCTION(), pwf);
  }
  if (this->LODMapper)
  {
    vtkInformation* mapperInfo = this->LODMapper->GetInformation();
    mapperInfo->Set(vtkOSPRayActorNode::SCALE_FUNCTION(), pwf);
  }
#else
  (void)pwf;
#endif
}
