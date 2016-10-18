/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturePainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTexturePainter.h"

#include "vtkCellType.h"
#include "vtkExtractVOI.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkTexture.h"

#include "vtkgl.h"

vtkStandardNewMacro(vtkTexturePainter);
vtkCxxSetObjectMacro(vtkTexturePainter, LookupTable, vtkScalarsToColors);
vtkInformationKeyMacro(vtkTexturePainter, SLICE, Integer);
vtkInformationKeyMacro(vtkTexturePainter, SLICE_MODE, Integer);
vtkInformationKeyMacro(vtkTexturePainter, LOOKUP_TABLE, ObjectBase);
vtkInformationKeyMacro(vtkTexturePainter, MAP_SCALARS, Integer);
vtkInformationKeyMacro(vtkTexturePainter, SCALAR_MODE, Integer);
vtkInformationKeyMacro(vtkTexturePainter, SCALAR_ARRAY_NAME, String);
vtkInformationKeyMacro(vtkTexturePainter, SCALAR_ARRAY_INDEX, Integer);
vtkInformationKeyMacro(vtkTexturePainter, USE_XY_PLANE, Integer);
//----------------------------------------------------------------------------
vtkTexturePainter::vtkTexturePainter()
{
  this->Texture = vtkTexture::New();
  this->Slice = 0;
  this->SliceMode = XY_PLANE;
  float* ptr = &this->QuadPoints[0][0];
  for (int cc = 0; cc < 12; cc++)
  {
    ptr[cc] = 0;
  }
  this->LookupTable = 0;
  this->MapScalars = 0;
  this->ScalarArrayName = 0;
  this->ScalarArrayIndex = 0;
  this->UseXYPlane = 0;
  this->ScalarMode = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = -1;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = -1;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = -1;
}

//----------------------------------------------------------------------------
vtkTexturePainter::~vtkTexturePainter()
{
  this->Texture->Delete();
  this->SetLookupTable(0);
  this->SetScalarArrayName(0);
}

//----------------------------------------------------------------------------
void vtkTexturePainter::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Texture->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkTexturePainter::ProcessInformation(vtkInformation* information)
{
  if (information->Has(SLICE()))
  {
    this->SetSlice(information->Get(SLICE()));
  }

  if (information->Has(SLICE_MODE()))
  {
    this->SetSliceMode(information->Get(SLICE_MODE()));
  }

  if (information->Has(LOOKUP_TABLE()))
  {
    vtkScalarsToColors* lut = vtkScalarsToColors::SafeDownCast(information->Get(LOOKUP_TABLE()));
    this->SetLookupTable(lut);
  }

  if (information->Has(MAP_SCALARS()))
  {
    this->SetMapScalars(information->Get(MAP_SCALARS()));
  }

  if (information->Has(SCALAR_MODE()))
  {
    this->SetScalarMode(information->Get(SCALAR_MODE()));
  }

  if (information->Has(SCALAR_ARRAY_NAME()))
  {
    this->SetScalarArrayName(information->Get(SCALAR_ARRAY_NAME()));
  }
  else
  {
    this->SetScalarArrayName(0);
  }

  if (information->Has(SCALAR_ARRAY_INDEX()))
  {
    this->SetScalarArrayIndex(information->Get(SCALAR_ARRAY_INDEX()));
  }

  if (information->Has(USE_XY_PLANE()))
  {
    this->SetUseXYPlane(information->Get(USE_XY_PLANE()));
  }
  else
  {
    this->SetUseXYPlane(0);
  }

  this->Superclass::ProcessInformation(information);
}

//----------------------------------------------------------------------------
static int vtkGetDataDimension(int inextents[6])
{
  int dim[3];
  dim[0] = inextents[1] - inextents[0] + 1;
  dim[1] = inextents[3] - inextents[2] + 1;
  dim[2] = inextents[5] - inextents[4] + 1;
  int dimensionality = 0;
  dimensionality += (dim[0] > 1 ? 1 : 0);
  dimensionality += (dim[1] > 1 ? 1 : 0);
  dimensionality += (dim[2] > 1 ? 1 : 0);
  return dimensionality;
}

static const int XY_PLANE_QPOINTS_INDICES[] = { 0, 2, 4, 1, 2, 4, 1, 3, 4, 0, 3, 4 };
static const int YZ_PLANE_QPOINTS_INDICES[] = { 0, 2, 4, 0, 3, 4, 0, 3, 5, 0, 2, 5 };
static const int XZ_PLANE_QPOINTS_INDICES[] = { 0, 2, 4, 1, 2, 4, 1, 2, 5, 0, 2, 5 };

static const int* XY_PLANE_QPOINTS_INDICES_ORTHO = XY_PLANE_QPOINTS_INDICES;
static const int YZ_PLANE_QPOINTS_INDICES_ORTHO[] = { 2, 4, 0, 3, 4, 0, 3, 5, 0, 2, 5, 0 };
static const int XZ_PLANE_QPOINTS_INDICES_ORTHO[] = { 4, 0, 2, 4, 1, 2, 5, 1, 2, 5, 0, 2 };

//----------------------------------------------------------------------------
int vtkTexturePainter::SetupScalars(vtkImageData* input)
{
  // Based on the scalar mode, scalar array, scalar id,
  // we need to tell the vtkTexture to use the appropriate scalars.
  int cellFlag = 0;
  vtkDataArray* scalars = vtkAbstractMapper::GetScalars(input, this->ScalarMode,
    this->ScalarArrayName ? VTK_GET_ARRAY_BY_NAME : VTK_GET_ARRAY_BY_ID, this->ScalarArrayIndex,
    this->ScalarArrayName, cellFlag);

  if (!scalars)
  {
    vtkWarningMacro("Failed to locate selected scalars. Will use image "
                    "scalars by default.");
    // If not scalar array specified, simply use the point data (the cell
    // data) scalars.
    this->Texture->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
    cellFlag = 0;
  }
  else
  {
    // Pass the scalar array choice to the texture.
    this->Texture->SetInputArrayToProcess(0, 0, 0,
      (cellFlag ? vtkDataObject::FIELD_ASSOCIATION_CELLS : vtkDataObject::FIELD_ASSOCIATION_POINTS),
      scalars->GetName());
  }
  return cellFlag;
}

#define PRINT(ext)                                                                                 \
  ext[0] << ",  " << ext[1] << ",  " << ext[2] << ",  " << ext[3] << ",  " << ext[4] << ",  "      \
         << ext[5]

//----------------------------------------------------------------------------
void vtkTexturePainter::RenderInternal(
  vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags, bool forceCompileOnly)
{
  vtkImageData* input = vtkImageData::SafeDownCast(this->GetInput());
  if (this->UpdateTime < input->GetMTime() || this->UpdateTime < this->MTime)
  {
    this->UpdateTime.Modified();
    int sliceDescription = 0;
    int inextent[6];
    int outextent[6];
    // we deliberately use whole extent here. So on processes where the slice is
    // not available, the vtkExtractVOI filter will simply yield an empty
    // output.
    memcpy(inextent, this->WholeExtent, 6 * sizeof(int));
    memcpy(outextent, inextent, sizeof(int) * 6);
    int numdims = ::vtkGetDataDimension(inextent);
    int dims[3];
    dims[0] = inextent[1] - inextent[0] + 1;
    dims[1] = inextent[3] - inextent[2] + 1;
    dims[2] = inextent[5] - inextent[4] + 1;

    // * Determine if we are using cell scalars or point scalars. That limits
    //   the extents/bounds etc.
    int cellFlag = this->SetupScalars(input);

    // Determine the VOI to extract:
    // * If the input image is 3D, then we respect the slice number and slice
    // direction the user recommended.
    // * If the input image is 2D, we simply show the input image slice.
    // * If the input image is 1D, we raise an error.
    if (numdims == 3)
    {
      int slice = this->Slice;
      // clamp the slice number at min val.
      slice = (slice < 0) ? 0 : slice;

      // if cell centered, then dimensions reduces by 1.
      int curdim = cellFlag ? (dims[this->SliceMode] - 1) : dims[this->SliceMode];

      // clamp the slice number at max val.
      slice = (slice >= curdim) ? curdim - 1 : slice;

      if (this->SliceMode == XY_PLANE) // XY plane
      {
        outextent[4] = outextent[5] = outextent[4] + slice;
        sliceDescription = VTK_XY_PLANE;
      }
      else if (this->SliceMode == YZ_PLANE) // YZ plane
      {
        outextent[0] = outextent[1] = outextent[0] + slice;
        sliceDescription = VTK_YZ_PLANE;
      }
      else if (this->SliceMode == XZ_PLANE) // XZ plane
      {
        outextent[2] = outextent[3] = outextent[2] + slice;
        sliceDescription = VTK_XZ_PLANE;
      }
    }
    else if (numdims == 2)
    {
      if (inextent[4] == inextent[5]) // XY plane
      {
        // nothing to change.
        sliceDescription = VTK_XY_PLANE;
      }
      else if (inextent[0] == inextent[1]) /// YZ plane
      {
        sliceDescription = VTK_YZ_PLANE;
      }
      else if (inextent[2] == inextent[3]) // XZ plane
      {
        sliceDescription = VTK_XZ_PLANE;
      }
    }
    else
    {
      vtkErrorMacro("Incorrect dimensionality.");
      return;
    }

    vtkSmartPointer<vtkImageData> clone = vtkSmartPointer<vtkImageData>::New();
    clone->ShallowCopy(input);

    vtkSmartPointer<vtkExtractVOI> extractVOI = vtkSmartPointer<vtkExtractVOI>::New();
    extractVOI->SetVOI(outextent);
    extractVOI->SetInputData(clone);
    extractVOI->Update();

    int evoi[6];
    extractVOI->GetOutput()->GetExtent(evoi);
    if (evoi[1] < evoi[0] && evoi[3] < evoi[2] && evoi[5] < evoi[4])
    {
      // if vtkExtractVOI did not produce a valid output, that means there's no
      // image slice to display.
      this->Texture->SetInputData(0);
      return;
    }

    // TODO: Here we would have change the input scalars if the user asked us to.
    // The LUT can be simply passed to the vtkTexture. It can handle scalar
    // mapping.
    this->Texture->SetInputConnection(extractVOI->GetOutputPort());
    double outputbounds[6];

    // TODO: vtkExtractVOI is not passing correct origin. Until that's fixed, I
    // will just use the input origin/spacing to compute the bounds.
    clone->SetExtent(evoi);
    clone->GetBounds(outputbounds);
    clone = 0;

    this->Texture->SetLookupTable(this->LookupTable);
    this->Texture->SetMapColorScalarsThroughLookupTable(this->MapScalars);

    if (cellFlag)
    {
      // Structured bounds are point bounds. Shrink them to reflect cell
      // center bounds.
      // i.e move min bounds up by spacing/2 in that direction
      //     and move max bounds down by spacing/2 in that direction.
      double spacing[3];
      input->GetSpacing(spacing); // since spacing doesn't change, we can use
                                  // input spacing directly.
      for (int dir = 0; dir < 3; dir++)
      {
        double& min = outputbounds[2 * dir];
        double& max = outputbounds[2 * dir + 1];
        if (min + spacing[dir] <= max)
        {
          min += spacing[dir] / 2.0;
          max -= spacing[dir] / 2.0;
        }
        else
        {
          min = max = (min + spacing[dir] / 2.0);
        }
      }
    }

    const int* indices = NULL;
    switch (sliceDescription)
    {
      case VTK_XY_PLANE:
        indices = XY_PLANE_QPOINTS_INDICES;
        if (this->UseXYPlane)
        {
          indices = XY_PLANE_QPOINTS_INDICES_ORTHO;
          outputbounds[4] = 0;
        }
        break;

      case VTK_YZ_PLANE:
        indices = YZ_PLANE_QPOINTS_INDICES;
        if (this->UseXYPlane)
        {
          indices = YZ_PLANE_QPOINTS_INDICES_ORTHO;
          outputbounds[0] = 0;
        }
        break;

      case VTK_XZ_PLANE:
        indices = XZ_PLANE_QPOINTS_INDICES;
        if (this->UseXYPlane)
        {
          indices = XZ_PLANE_QPOINTS_INDICES_ORTHO;
          outputbounds[2] = 0;
        }
        break;
    }

    for (int cc = 0; cc < 12; cc++)
    {
      static_cast<float*>(&this->QuadPoints[0][0])[cc] = outputbounds[indices[cc]];
    }
  }

  if (!this->Texture->GetInput())
  {
    return;
  }

  vtkPainterDeviceAdapter* device = renderer->GetRenderWindow()->GetPainterDeviceAdapter();

  // Lighting needs to be disabled since this painter always employs scalar
  // coloring and when coloring with scalars we always disable lighting.
  GLboolean lighting = glIsEnabled(GL_LIGHTING);
  if (lighting)
  {
    glDisable(GL_LIGHTING);
  }

  this->Texture->Load(renderer);

  float tcoords[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
  device->BeginPrimitive(VTK_QUAD);
  for (int cc = 0; cc < 4; cc++)
  {
    device->SendAttribute(vtkPointData::TCOORDS, 2, VTK_FLOAT, &tcoords[cc][0], 0);
    device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3, VTK_FLOAT, &this->QuadPoints[cc][0], 0);
  }
  device->EndPrimitive();

  // restore lighting if needed
  if (lighting)
  {
    glEnable(GL_LIGHTING);
  }

  this->Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);
}

//----------------------------------------------------------------------------
void vtkTexturePainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Slice: " << this->Slice << endl;
  os << indent << "SliceMode: " << this->SliceMode << endl;
  os << indent << "MapScalars: " << this->MapScalars << endl;
  os << indent << "ScalarMode: ";
  switch (this->ScalarMode)
  {
    case VTK_SCALAR_MODE_DEFAULT:
      os << "DEFAULT";
      break;

    case VTK_SCALAR_MODE_USE_POINT_DATA:
      os << "USE POINT DATA";
      break;

    case VTK_SCALAR_MODE_USE_CELL_DATA:
      os << "USE CELL DATA";
      break;

    case VTK_SCALAR_MODE_USE_POINT_FIELD_DATA:
      os << "USE POINT FIELD DATA";
      break;

    case VTK_SCALAR_MODE_USE_CELL_FIELD_DATA:
      os << "USE CELL FIELD DATA";
      break;

    case VTK_SCALAR_MODE_USE_FIELD_DATA:
      os << "USE FIELD DATA";
      break;

    default:
      os << "INVALID";
  }
  os << endl;
  os << indent << "ScalarArrayName: " << (this->ScalarArrayName ? this->ScalarArrayName : "(none)")
     << endl;
  os << indent << "ScalarArrayIndex: " << this->ScalarArrayIndex << endl;
  os << indent << "LookupTable: " << this->LookupTable << endl;
}
