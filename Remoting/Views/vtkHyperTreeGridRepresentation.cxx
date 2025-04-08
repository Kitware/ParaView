// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSet.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLHyperTreeGridMapper.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVRenderView.h"
#include "vtkPVTrivialProducer.h"
#include "vtkProcessModule.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSelection.h"
#include "vtkShader.h"
#include "vtkShaderProperty.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayActorNode.h"
#endif

#include <unordered_map>
#include <vtk_jsoncpp.h>
#include <vtksys/SystemTools.hxx>

//*****************************************************************************

vtkStandardNewMacro(vtkHyperTreeGridRepresentation);
//----------------------------------------------------------------------------

vtkHyperTreeGridRepresentation::vtkHyperTreeGridRepresentation()
{
  vtkMath::UninitializeBounds(this->VisibleDataBounds);
  this->SetupDefaults();
}

//----------------------------------------------------------------------------
vtkHyperTreeGridRepresentation::~vtkHyperTreeGridRepresentation() = default;

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetupDefaults()
{
  vtkNew<vtkSelection> sel;
  this->Mapper->SetSelection(sel);

  // Required to set block visiblities for composite inputs
  vtkNew<vtkCompositeDataDisplayAttributes> compositeAttributes;
  this->Mapper->SetCompositeDataDisplayAttributes(compositeAttributes);

  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  vtkNew<vtkInformation> keys;
  this->Actor->SetPropertyKeys(keys);
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridRepresentation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");

  // Saying INPUT_IS_OPTIONAL() is essential, since representations don't have
  // any inputs on client-side (in client-server, client-render-server mode) and
  // render-server-side (in client-render-server mode).
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    // i.e. this->GetVisibility() == false, hence nothing to do.
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    // provide the "geometry" to the view so the view can delivery it to the
    // rendering nodes as and when needed.
    vtkPVView::SetPiece(inInfo, this, this->GetInput(0));

    // We want to let vtkPVRenderView do redistribution of data as necessary,
    // and use this representations data for determining a load balanced distribution
    // if ordered is needed.
    vtkPVRenderView::SetOrderedCompositingConfiguration(inInfo, this,
      vtkPVRenderView::DATA_IS_REDISTRIBUTABLE | vtkPVRenderView::USE_DATA_FOR_LOAD_BALANCING);

    outInfo->Set(
      vtkPVRenderView::NEED_ORDERED_COMPOSITING(), this->NeedsOrderedCompositing() ? 1 : 0);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    auto data = vtkPVView::GetDeliveredPiece(inInfo, this);
    this->Mapper->SetInputDataObject(data);
    this->Mapper->SetUseAdaptiveDecimation(this->AdaptiveDecimation);

    // Toggle block visibility in the mapper based on block selectors for composite inputs
    auto dtree = vtkDataObjectTree::SafeDownCast(data);
    if (dtree)
    {
      this->UpdateBlockVisibility(dtree);
    }

    // This is called just before the vtk-level render. In this pass, we simply
    // pick the correct rendering mode and rendering parameters.
    // (when interactive LOD in PV for example)
    this->UpdateColoringParameters();
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      vtkAlgorithmOutput* aOut = this->GetInternalOutputPort();
      vtkPVTrivialProducer* prod = vtkPVTrivialProducer::SafeDownCast(aOut->GetProducer());
      if (prod)
      {
        prod->SetWholeExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
      }
    }
    this->Mapper->SetInputConnection(this->GetInternalOutputPort());

    // Needed so that the mapper can update its bounds based on the data during the first render
    vtkDataObject* input = vtkDataObject::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    this->Mapper->SetInputDataObject(input);
  }

  this->Mapper->SetScalarModeToUseCellData();

  // essential to re-execute AdaptiveDecimation filter consistently on all ranks since it
  // does use parallel communication (see #19963).
  this->Mapper->Modified();
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHyperTreeGridRepresentation::GetRenderedDataObject(int vtkNotUsed(port))
{
  return this->Mapper->GetInput();
}

//----------------------------------------------------------------------------
bool vtkHyperTreeGridRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);

    // Indicate that this is prop that we are rendering when hardware selection
    // is enabled.
    rview->RegisterPropForHardwareSelection(this, this->GetRenderedProp());
    return this->Superclass::AddToView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkHyperTreeGridRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->RemoveActor(this->Actor);
    rview->UnRegisterPropForHardwareSelection(this, this->GetRenderedProp());
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetRepresentation(const char* type)
{
  if (vtksys::SystemTools::Strucmp(type, "Wireframe") == 0)
  {
    this->SetRepresentation(WIREFRAME);
  }
  else if (vtksys::SystemTools::Strucmp(type, "Surface") == 0)
  {
    this->SetRepresentation(SURFACE);
  }
  else if (vtksys::SystemTools::Strucmp(type, "Surface With Edges") == 0)
  {
    this->SetRepresentation(SURFACE_WITH_EDGES);
  }
  else
  {
    vtkErrorMacro("Invalid representation type: " << type);
  }
}

//----------------------------------------------------------------------------
const char* vtkHyperTreeGridRepresentation::GetColorArrayName()
{
  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    return info->Get(vtkDataObject::FIELD_NAME());
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::UpdateBlockVisibility(vtkDataObjectTree* dtree)
{
  const vtkNew<vtkDataAssembly> hierarchy;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(dtree, hierarchy))
  {
    return;
  }

  // The mapper needs a compositeAttributes in order to set block visibility
  vtkNew<vtkCompositeDataDisplayAttributes> compositeAttributes;
  this->Mapper->SetCompositeDataDisplayAttributes(compositeAttributes);

  // compute the composite ids for the hierarchy selectors
  this->Mapper->RemoveBlockVisibilities();
  this->Mapper->SetBlockVisibility(hierarchy->GetRootNode(), false); // Hide root first
  std::vector<unsigned int> cids =
    vtkDataAssemblyUtilities::GetSelectedCompositeIds(this->BlockSelectors, hierarchy);
  for (const auto& id : cids)
  {
    this->Mapper->SetBlockVisibility(id, true);
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::UpdateColoringParameters()
{
  bool usingScalarColoring = false;

  vtkInformation* info = this->GetInputArrayInformation(0);

  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    const char* colorArrayName = info->Get(vtkDataObject::FIELD_NAME());
    int fieldAssociation = info->Get(vtkDataObject::FIELD_ASSOCIATION());
    if (colorArrayName && colorArrayName[0])
    {
      this->Mapper->SetScalarVisibility(1);
      this->Mapper->SelectColorArray(colorArrayName);
      this->Mapper->SetUseLookupTableScalarRange(1);
      switch (fieldAssociation)
      {
        case vtkDataObject::FIELD_ASSOCIATION_NONE:
          this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
          // Color entire block by first tuple in the field data
          this->Mapper->SetFieldDataTupleId(0);
          break;

        case vtkDataObject::FIELD_ASSOCIATION_POINTS: // no point data in HTG
        case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        default:
          this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
          break;
      }
      usingScalarColoring = true;
    }
  }

  if (!usingScalarColoring)
  {
    this->Mapper->SetScalarVisibility(0);
    this->Mapper->SelectColorArray(nullptr);
  }

  // Adjust material properties.
  this->Property->SetAmbient(this->Ambient);
  this->Property->SetSpecular(this->Specular);
  this->Property->SetDiffuse(this->Diffuse);

  switch (this->Representation)
  {
    case SURFACE_WITH_EDGES:
      this->Property->SetEdgeVisibility(1);
      this->Property->SetRepresentation(VTK_SURFACE);
      break;
    case SURFACE:
      this->Property->SetEdgeVisibility(0);
      this->Property->SetRepresentation(VTK_SURFACE);
      break;
    case WIREFRAME:
      this->Property->SetRepresentation(VTK_WIREFRAME);
      break;
    default:
      vtkErrorMacro("Invalid representation: " << this->Representation);
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val);
  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
bool vtkHyperTreeGridRepresentation::NeedsOrderedCompositing()
{
  // One would think simply calling `vtkActor::HasTranslucentPolygonalGeometry`
  // should do the trick, however that method relies on the mapper's input
  // having up-to-date data. vtkHyperTreeGridRepresentation needs to determine
  // whether the representation needs ordered compositing in `REQUEST_UPDATE`
  // pass i.e. before the mapper's input is updated. Hence we explicitly
  // determine if the mapper may choose to render translucent geometry.
  if (this->Actor->GetForceOpaque())
  {
    return false;
  }

  if (this->Actor->GetForceTranslucent())
  {
    return true;
  }

  if (auto prop = this->Actor->GetProperty())
  {
    auto opacity = prop->GetOpacity();
    if (opacity > 0.0 && opacity < 1.0)
    {
      return true;
    }
  }

  if (auto texture = this->Actor->GetTexture())
  {
    if (texture->IsTranslucent())
    {
      return true;
    }
  }

  auto colorarrayname = this->GetColorArrayName();
  if (colorarrayname && colorarrayname[0])
  {
    if (this->Mapper->GetColorMode() == VTK_COLOR_MODE_DIRECT_SCALARS)
    {
      // when mapping scalars directly, assume the scalars have an alpha
      // component since we cannot check if that is indeed the case consistently
      // on all ranks without a bit of work.
      return true;
    }

    if (auto lut = this->Mapper->GetLookupTable())
    {
      if (lut->IsOpaque() == 0)
      {
        return true;
      }
    }
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // HTG specific
  os << indent << "AdaptiveDecimation: " << this->AdaptiveDecimation << std::endl;
  os << indent << "OpenGL HTG Mapper: " << std::endl;
  this->Mapper->PrintSelf(os, indent.GetNextIndent());

  // Representation
  os << indent << "Representation: " << this->Representation << std::endl;
  os << indent << "Internal Actor: " << std::endl;
  this->Actor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Internal Property: " << std::endl;
  this->Property->PrintSelf(os, indent.GetNextIndent());
  os << indent << "RepeatTextures: " << this->RepeatTextures << std::endl;
  os << indent << "InterpolateTextures: " << this->InterpolateTextures << std::endl;
  os << indent << "UseMipmapTextures: " << this->UseMipmapTextures << std::endl;
  os << indent << "Ambient: " << this->Ambient << std::endl;
  os << indent << "Specular: " << this->Specular << std::endl;
  os << indent << "Diffuse: " << this->Diffuse << std::endl;
  os << indent << "VisibleDataBounds: " << this->VisibleDataBounds << std::endl;
  os << indent << "UseShaderReplacements: " << this->UseShaderReplacements << std::endl;
  os << indent << "ShaderReplacementsString: " << this->ShaderReplacementsString << std::endl;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetUseOutline(int vtkNotUsed(val))
{
  vtkWarningMacro("Outline not supported by the HTG Representation.");
}

//****************************************************************************
// Methods merely forwarding parameters to internal objects.
//****************************************************************************

//----------------------------------------------------------------------------
// Forwarded to vtkProperty
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetAmbientColor(double r, double g, double b)
{
  this->Property->SetAmbientColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetBaseIOR(double val)
{
  this->Property->SetBaseIOR(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetBaseColorTexture(vtkTexture* tex)
{
  if (tex)
  {
    tex->UseSRGBColorSpaceOn();
    tex->SetInterpolate(this->InterpolateTextures);
    tex->SetRepeat(this->RepeatTextures);
    tex->SetMipmap(this->UseMipmapTextures);
  }
  this->Property->SetBaseColorTexture(tex);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetCoatIOR(double val)
{
  this->Property->SetCoatIOR(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetCoatStrength(double val)
{
  this->Property->SetCoatStrength(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetCoatRoughness(double val)
{
  this->Property->SetCoatRoughness(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetCoatColor(double a, double b, double c)
{
  this->Property->SetCoatColor(a, b, c);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetCoatNormalScale(double val)
{
  this->Property->SetCoatNormalScale(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetColor(double r, double g, double b)
{
  this->Property->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetDiffuseColor(double r, double g, double b)
{
  this->Property->SetDiffuseColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetEdgeColor(double r, double g, double b)
{
  this->Property->SetEdgeColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetEdgeTint(double r, double g, double b)
{
  this->Property->SetEdgeTint(r, g, b);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetEmissiveFactor(double rval, double gval, double bval)
{
  this->Property->SetEmissiveFactor(rval, gval, bval);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetEmissiveTexture(vtkTexture* tex)
{
  if (tex)
  {
    tex->UseSRGBColorSpaceOn();
    tex->SetInterpolate(this->InterpolateTextures);
    tex->SetRepeat(this->RepeatTextures);
    tex->SetMipmap(this->UseMipmapTextures);
  }
  this->Property->SetEmissiveTexture(tex);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetInteractiveSelectionColor(double r, double g, double b)
{
  this->Property->SetSelectionColor(r, g, b, 1.0);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetInterpolation(int val)
{
  this->Property->SetInterpolation(val);
}
//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetLineWidth(double val)
{
  this->Property->SetLineWidth(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetMaterial(std::string name)
{
  this->Property->SetMaterialName(name.c_str());
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetMaterialTexture(vtkTexture* tex)
{
  if (tex)
  {
    tex->UseSRGBColorSpaceOff();
    tex->SetInterpolate(this->InterpolateTextures);
    tex->SetRepeat(this->RepeatTextures);
    tex->SetMipmap(this->UseMipmapTextures);
  }
  this->Property->SetORMTexture(tex);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetMetallic(double val)
{
  this->Property->SetMetallic(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetNormalScale(double val)
{
  this->Property->SetNormalScale(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetNormalTexture(vtkTexture* tex)
{
  if (tex)
  {
    tex->UseSRGBColorSpaceOff();
    tex->SetInterpolate(this->InterpolateTextures);
    tex->SetRepeat(this->RepeatTextures);
    tex->SetMipmap(this->UseMipmapTextures);
  }
  this->Property->SetNormalTexture(tex);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetOcclusionStrength(double val)
{
  this->Property->SetOcclusionStrength(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetOpacity(double val)
{
  this->Property->SetOpacity(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetPointSize(double val)
{
  this->Property->SetPointSize(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetRenderLinesAsTubes(bool val)
{
  this->Property->SetRenderLinesAsTubes(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetRenderPointsAsSpheres(bool val)
{
  this->Property->SetRenderPointsAsSpheres(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetRoughness(double val)
{
  this->Property->SetRoughness(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetSpecularColor(double r, double g, double b)
{
  this->Property->SetSpecularColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetSpecularPower(double val)
{
  this->Property->SetSpecularPower(val);
}

//----------------------------------------------------------------------------
// Actor
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetFlipTextures(bool flip)
{
  vtkInformation* info = this->Actor->GetPropertyKeys();
  info->Remove(vtkProp::GeneralTextureTransform());
  if (flip)
  {
    double mat[] = { 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    info->Set(vtkProp::GeneralTextureTransform(), mat, 16);
  }
  this->Actor->Modified();
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetPosition(double x, double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetTexture(vtkTexture* val)
{
  this->Actor->SetTexture(val);
  if (val)
  {
    val->SetRepeat(this->RepeatTextures);
    val->SetInterpolate(this->InterpolateTextures);
    val->SetMipmap(this->UseMipmapTextures);
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetUserTransform(const double matrix[16])
{
  vtkNew<vtkTransform> transform;
  transform->SetMatrix(matrix);
  this->Actor->SetUserTransform(transform);
}

//----------------------------------------------------------------------------
// OSPray
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetEnableScaling(int val)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  this->Actor->SetEnableScaling(val);
#else
  (void)val;
#endif
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetScalingArrayName(const char* val)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  this->Actor->SetScalingArrayName(val);
#else
  (void)val;
#endif
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetScalingFunction(vtkPiecewiseFunction* pwf)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  this->Actor->SetScalingFunction(pwf);
#else
  (void)pwf;
#endif
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetMaterial(const char* val)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  if (!strcmp(val, "None"))
  {
    this->Property->SetMaterialName(nullptr);
  }
  else
  {
    this->Property->SetMaterialName(val);
  }
#else
  (void)val;
#endif
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetLuminosity(double val)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayActorNode::SetLuminosity(val, this->Property);
#else
  (void)val;
#endif
}

//----------------------------------------------------------------------------
// Texture
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetRepeatTextures(bool rep)
{
  if (this->Actor->GetTexture())
  {
    this->Actor->GetTexture()->SetRepeat(rep);
  }
  std::map<std::string, vtkTexture*>& tex = this->Actor->GetProperty()->GetAllTextures();
  for (auto t : tex)
  {
    t.second->SetRepeat(rep);
  }
  this->RepeatTextures = rep;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetInterpolateTextures(bool rep)
{
  if (this->Actor->GetTexture())
  {
    this->Actor->GetTexture()->SetInterpolate(rep);
  }
  std::map<std::string, vtkTexture*>& tex = this->Actor->GetProperty()->GetAllTextures();
  for (auto t : tex)
  {
    t.second->SetInterpolate(rep);
  }
  this->InterpolateTextures = rep;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetUseMipmapTextures(bool rep)
{
  if (this->Actor->GetTexture())
  {
    this->Actor->GetTexture()->SetMipmap(rep);
  }
  std::map<std::string, vtkTexture*>& tex = this->Actor->GetProperty()->GetAllTextures();
  for (auto t : tex)
  {
    t.second->SetMipmap(rep);
  }
  this->UseMipmapTextures = rep;
}

//----------------------------------------------------------------------------
// Mapper and LODMapper
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetInterpolateScalarsBeforeMapping(int val)
{
  // XXX This has no effect on HTG as they only have cell data
  this->Mapper->SetInterpolateScalarsBeforeMapping(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetLookupTable(vtkScalarsToColors* val)
{
  this->Mapper->SetLookupTable(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetMapScalars(bool val)
{
  this->Mapper->SetColorMode(val ? VTK_COLOR_MODE_MAP_SCALARS : VTK_COLOR_MODE_DIRECT_SCALARS);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetStatic(int val)
{
  this->Mapper->SetStatic(val);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetSelection(vtkSelection* selection)
{
  // we need to shallow copy the existing selection instead of changing it in order to avoid
  // changing the MTime of the mapper to avoid rebuilding everything
  this->Mapper->GetSelection()->ShallowCopy(selection);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetUseShaderReplacements(bool useShaderRepl)
{
  if (this->UseShaderReplacements != useShaderRepl)
  {
    this->UseShaderReplacements = useShaderRepl;
    this->Modified();
    this->UpdateShaderReplacements();
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::SetShaderReplacements(const char* replacementsString)
{
  if (replacementsString != this->ShaderReplacementsString)
  {
    this->ShaderReplacementsString = std::string(replacementsString);
    this->Modified();
    this->UpdateShaderReplacements();
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::UpdateShaderReplacements()
{
  vtkShaderProperty* props = this->Actor->GetShaderProperty();

  if (!props)
  {
    return;
  }

  props->ClearAllShaderReplacements();

  if (!this->UseShaderReplacements || this->ShaderReplacementsString.empty())
  {
    return;
  }

  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;
  Json::Value root;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  bool success = reader->parse(this->ShaderReplacementsString.c_str(),
    this->ShaderReplacementsString.c_str() + this->ShaderReplacementsString.length(), &root,
    nullptr);
  if (!success)
  {
    vtkGenericWarningMacro("Unable to parse the replacement Json string!");
    return;
  }
  bool isArray = root.isArray();
  size_t nbReplacements = isArray ? root.size() : 1;

  std::vector<std::tuple<vtkShader::Type, std::string, std::string>> replacements;
  for (size_t index = 0; index < nbReplacements; ++index)
  {
    const Json::Value& repl = isArray ? root[(int)index] : root;
    if (!repl.isMember("type"))
    {
      vtkErrorMacro("Syntax error in shader replacements: a type is required.");
      return;
    }
    std::string type = repl["type"].asString();
    vtkShader::Type shaderType = vtkShader::Unknown;
    if (type == "fragment")
    {
      shaderType = vtkShader::Fragment;
    }
    else if (type == "vertex")
    {
      shaderType = vtkShader::Vertex;
    }
    else if (type == "geometry")
    {
      shaderType = vtkShader::Geometry;
    }
    if (shaderType == vtkShader::Unknown)
    {
      vtkErrorMacro("Unknown shader type for replacement:" << type);
      return;
    }

    if (!repl.isMember("original"))
    {
      vtkErrorMacro("Syntax error in shader replacements: an original pattern is required.");
      return;
    }
    std::string original = repl["original"].asString();
    if (!repl.isMember("replacement"))
    {
      vtkErrorMacro("Syntax error in shader replacements: a replacement pattern is required.");
      return;
    }
    std::string replacement = repl["replacement"].asString();
    replacements.push_back(std::make_tuple(shaderType, original, replacement));
  }

  for (const auto& r : replacements)
  {
    switch (std::get<0>(r))
    {
      case vtkShader::Fragment:
        props->AddFragmentShaderReplacement(std::get<1>(r), true, std::get<2>(r), true);
        break;
      case vtkShader::Vertex:
        props->AddVertexShaderReplacement(std::get<1>(r), true, std::get<2>(r), true);
        break;
      case vtkShader::Geometry:
        props->AddGeometryShaderReplacement(std::get<1>(r), true, std::get<2>(r), true);
        break;
      default:
        assert(false && "unknown shader replacement type");
        break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::AddBlockSelector(const char* selector)
{
  if (selector != nullptr &&
    std::find(this->BlockSelectors.begin(), this->BlockSelectors.end(), selector) ==
      this->BlockSelectors.end())
  {
    this->BlockSelectors.emplace_back(selector);
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridRepresentation::RemoveAllBlockSelectors()
{
  this->BlockSelectors.clear();
}
