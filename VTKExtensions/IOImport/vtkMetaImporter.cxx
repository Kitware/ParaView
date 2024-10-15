// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMetaImporter.h"
#include "vtk3DSImporter.h"
#include "vtkAOSDataArrayTemplate.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkDistributedTrivialProducer.h"
#include "vtkEventForwarderCommand.h"
#include "vtkGLTFImporter.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkLinearTransform.h"
#include "vtkLogger.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkOBJImporter.h"
#include "vtkObjectFactory.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"

#include <vtksys/SystemTools.hxx>

// clang-format off
#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

#include <map>
#include <set>

namespace
{
//----------------------------------------------------------------------------
template <typename ValueT>
nlohmann::json to_json(ValueT* array, const std::size_t numElements)
{
  return nlohmann::json(std::vector<ValueT>(array, array + numElements));
}

//----------------------------------------------------------------------------
nlohmann::json to_json(vtkProperty* ppty)
{
  // clang-format off
  return
  {
    { "Color", to_json(ppty->GetColor(), 3) },
    { "AmbientColor", to_json(ppty->GetAmbientColor(), 3) },
    { "DiffuseColor", to_json(ppty->GetDiffuseColor(), 3) },
    { "SpecularColor", to_json(ppty->GetSpecularColor(), 3) },
    { "EdgeColor", to_json(ppty->GetEdgeColor(), 3) },
    { "VertexColor", to_json(ppty->GetVertexColor(), 3) },
    { "SelectionColor", to_json(ppty->GetSelectionColor(), 4) },
    { "Ambient", ppty->GetAmbient() },
    { "Diffuse", ppty->GetDiffuse() },
    { "Metallic", ppty->GetMetallic() },
    { "Roughness", ppty->GetRoughness() },
    { "Anisotropy", ppty->GetAnisotropy() },
    { "AnisotropyRotation", ppty->GetAnisotropyRotation() },
    { "BaseIOR", ppty->GetBaseIOR() },
    { "CoatIOR", ppty->GetCoatIOR() },
    { "CoatColor", to_json(ppty->GetCoatColor(), 3) },
    { "CoatRoughness", ppty->GetCoatRoughness() },
    { "CoatStrength", ppty->GetCoatStrength() },
    { "CoatNormalScale", ppty->GetCoatNormalScale() },
    { "NormalScale", ppty->GetNormalScale() },
    { "OcclusionStrength", ppty->GetOcclusionStrength() },
    { "EmissiveFactor", to_json(ppty->GetEmissiveFactor(), 3) },
    { "Specular", ppty->GetSpecular() },
    { "SpecularPower", ppty->GetSpecularPower() },
    { "Opacity", ppty->GetOpacity() },
    { "EdgeOpacity", ppty->GetEdgeOpacity() },
    { "EdgeTint", to_json(ppty->GetEdgeTint(), 3) },
    { "PointSize", ppty->GetPointSize() },
    { "LineWidth", ppty->GetLineWidth() },
    { "SelectionPointSize", ppty->GetSelectionPointSize() },
    { "SelectionLineWidth", ppty->GetSelectionLineWidth() },
    { "LineStipplePattern", ppty->GetLineStipplePattern() },
    { "LineStippleRepeatFactor", ppty->GetLineStippleRepeatFactor() },
    { "Interpolation", ppty->GetInterpolation() },
    { "Representation", ppty->GetRepresentation() },
    { "EdgeVisibility", ppty->GetEdgeVisibility() },
    { "VertexVisibility", ppty->GetVertexVisibility() },
    { "BackfaceCulling", ppty->GetBackfaceCulling() },
    { "FrontfaceCulling", ppty->GetFrontfaceCulling() },
    { "Lighting", ppty->GetLighting() },
    { "RenderPointsAsSpheres", ppty->GetRenderPointsAsSpheres() },
    { "RenderLinesAsTubes", ppty->GetRenderLinesAsTubes() },
    { "ShowTexturesOnBackface", ppty->GetShowTexturesOnBackface() },
    { "Shading", ppty->GetShading() },
    { "MaterialName", ppty->GetMaterialName() ? ppty->GetMaterialName() : "" },
    { "Textures", 
      nlohmann::json
      (
        {
          { "albedoTex", ppty->GetTexture("albedoTex") != nullptr },
          { "materialTex", ppty->GetTexture("materialTex") != nullptr },
          { "anisotropyTex", ppty->GetTexture("anisotropyTex") != nullptr },
          { "normalTex", ppty->GetTexture("normalTex") != nullptr },
          { "emissiveTex", ppty->GetTexture("emissiveTex") != nullptr },
          { "coatNormalTex", ppty->GetTexture("coatNormalTex") != nullptr },
        }
      )
    }
  };
  // clang-format on
}

//----------------------------------------------------------------------------
nlohmann::json to_json(vtkActor* actor, const std::string& name)
{
  bool flipTextures = true;
  if (actor->GetPropertyKeys())
  {
    // determine whether importer flipped the textures.
    double* textureTransform = actor->GetPropertyKeys()->Get(vtkProp::GeneralTextureTransform());
    const double mat[] = { 1, 0, 0, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1 };
    for (int i = 0; i < 16; ++i)
    {
      flipTextures &= (textureTransform[i] == mat[i]);
    }
  }
  // clang-format off
  return 
  { 
    { "Name", name },
    { "IsIdentity", actor->GetIsIdentity() },
    { "DisplayProperty", to_json(actor->GetProperty()) },
    { "UserTransform", to_json(actor->GetUserTransform()->GetMatrix()->GetData(), 16) },
    { "Origin", to_json(actor->GetOrigin(), 3) },
    { "Translation", to_json(actor->GetPosition(), 3) },
    { "Orientation", to_json(actor->GetOrientation(), 3) },
    { "Scale", to_json(actor->GetScale(), 3) },
    { "Center", to_json(actor->GetCenter(), 3) },
    { "FlipTextures", flipTextures }
  };
  // clang-format on
}

//----------------------------------------------------------------------------
nlohmann::json to_json(vtkCamera* camera)
{
  // clang-format off
  return
  {
    { "Position", to_json(camera->GetPosition(), 3) },
    { "FocalPoint", to_json(camera->GetFocalPoint(), 2) },
    { "ViewUp", to_json(camera->GetViewUp(), 3) },
    { "ViewAngle", camera->GetViewAngle() },
    { "FocalDisk", camera->GetFocalDisk() },
    { "FocalDistance", camera->GetFocalDistance() },
    { "ParallelScale", camera->GetParallelScale() },
    { "EyeAngle", camera->GetEyeAngle() },
    { "ParallelProjection", camera->GetParallelProjection() },
    { "EyeTransformMatrix", to_json(camera->GetEyeTransformMatrix()->GetData(), 16) },
    { "ModelTransformMatrix", to_json(camera->GetModelTransformMatrix()->GetData(), 16) }
  };
  // clang-format on
}

//----------------------------------------------------------------------------
nlohmann::json to_json(vtkLight* light, const std::string& name)
{
  // clang-format off
  return
  {
    { "Name", name },
    { "FocalPoint", to_json(light->GetFocalPoint(), 3) },
    { "Position", to_json(light->GetPosition(), 3) },
    { "Intensity", light->GetIntensity() },
    { "AmbientColor", to_json(light->GetAmbientColor(), 3) },
    { "DiffuseColor", to_json(light->GetDiffuseColor(), 3) },
    { "SpecularColor", to_json(light->GetSpecularColor(), 3) },
    { "Switch", light->GetSwitch() },
    { "Positional", light->GetPositional() },
    { "Exponent", light->GetExponent() },
    { "ConeAngle", light->GetConeAngle() },
    { "AttenuationValues", to_json(light->GetAttenuationValues(), 3) },
    { "TransformMatrix", to_json(light->GetTransformMatrix()->GetData(), 16) },
    { "LightType", light->GetLightType() },
    { "ShadowAttenuation", light->GetShadowAttenuation() }
  };
  // clang-format on
}

//----------------------------------------------------------------------------
nlohmann::json to_json(vtkLightCollection* lights)
{
  vtkCollectionSimpleIterator pit;
  lights->InitTraversal(pit);
  auto lightsJson = nlohmann::json::array();
  int lightId = 0;
  while (auto light = vtkLight::SafeDownCast(lights->GetNextLight(pit)))
  {
    const std::string name = "Light" + std::to_string(lightId++);
    lightsJson.push_back(to_json(light, name));
  }
  return lightsJson;
}

//----------------------------------------------------------------------------
/// output adapter for vtkAOSDataArrayTemplate<CharType>
template <typename CharType, typename ArrayType = vtkAOSDataArrayTemplate<CharType>>
class output_vtk_buffer_adapter : public nlohmann::detail::output_adapter_protocol<CharType>
{
  vtkSmartPointer<ArrayType> Array;

public:
  explicit output_vtk_buffer_adapter(vtkSmartPointer<ArrayType> array) noexcept
    : Array(array)
  {
  }

  void write_character(CharType value) override { this->Array->InsertNextValue(value); }

  void write_characters(const CharType* values, std::size_t length) override
  {
    for (std::size_t i = 0; i < length; ++i)
    {
      this->Array->InsertNextValue(values[i]);
    }
  }
};
}

class vtkMetaImporter::vtkInternals
{
private:
  class SceneNodeSerializer : public vtkDataAssemblyVisitor
  {
    std::map<vtkDataObject*, std::string> Cache;
    vtkMetaImporter::vtkInternals* Self = nullptr;

    SceneNodeSerializer(const SceneNodeSerializer&) = delete;
    void operator=(const SceneNodeSerializer&) = delete;

  protected:
    SceneNodeSerializer() = default;
    ~SceneNodeSerializer() override = default;

  public:
    nlohmann::json::array_t ActorsJson;

    static SceneNodeSerializer* New() { VTK_STANDARD_NEW_BODY(SceneNodeSerializer); }

    vtkTypeMacro(SceneNodeSerializer, vtkDataAssemblyVisitor);

    void PrintSelf(ostream& os, vtkIndent indent) override
    {
      this->Superclass::PrintSelf(os, indent);
    }

    void Initialize(vtkMetaImporter::vtkInternals* self) { this->Self = self; }

    void Visit(int nodeId) override
    {
      if (this->Self == nullptr)
      {
        vtkLog(ERROR, << vtkLogIdentifier(this)
                      << " visit called but visitor is not initialized correctly!");
        return;
      }
      auto hierarchy = this->GetAssembly();
      const int flatActorId = hierarchy->GetAttributeOrDefault(nodeId, "flat_actor_id", -1);
      if (flatActorId < 0)
      {
        return;
      }
      const char* parentNodeName = hierarchy->GetAttributeOrDefault(nodeId, "parent_node_name", "");
      auto props = this->Self->Importer->GetRenderer()->GetViewProps();
      if (auto actor = vtkActor::SafeDownCast(props->GetItemAsObject(flatActorId)))
      {
        if (auto mapper = actor->GetMapper())
        {
          // actor name includes parent node name and mesh name to disambiguate actors
          // that share a common mesh but different display properties or transform matrices.
          const std::string actorName =
            std::string(parentNodeName) + "_" + hierarchy->GetNodeName(nodeId);
          auto actorJson = to_json(actor, actorName);
          auto ppty = actor->GetProperty();
          // attach cache information that tells client how to find the input geometry.
          const auto& inputDOKey = this->Self->CacheDataObject(
            mapper->GetInputDataObject(0, 0), actorName + "_Geometry", this->Cache);
          actorJson["InputDataObjectKey"] = inputDOKey;
          for (auto& texInfo : ppty->GetAllTextures())
          {
            const auto& textureName = texInfo.first;
            const auto texture = texInfo.second;
            if (texture != nullptr)
            {
              auto& displayPropertyJson = actorJson["DisplayProperty"];
              auto& texturesJson = displayPropertyJson["Textures"];
              auto image = texture->GetInputDataObject(0, 0);
              const auto& inputImgKey = this->Self->CacheDataObject(
                image, actorName + "_" + textureName + "_Image", this->Cache);
              // attach cache information that tells client how to find the input image.
              texturesJson[textureName + "InputDataObjectKey"] = inputImgKey;
            }
          }
          this->ActorsJson.emplace_back(actorJson);
        }
      }
    }
  };
  friend class SceneNodeSerializer;

public:
  vtkSmartPointer<vtkImporter> Importer;
  vtkSmartPointer<PayloadType> SceneDescription;
  std::set<std::string> NodeSelectors;
  std::vector<std::string> DataObjectCacheKeys;
  unsigned long ProgressEventObserverTag = 0;

  //----------------------------------------------------------------------------
  nlohmann::json SerializePropCollection()
  {
    // A mapper's input dataset and a texture's input image data are all stashed inside
    // vtkDistributedTrivialProducer with a unique key. This way, client applications
    // can refer back to the datasets which provide mesh data for a representation or
    // an image for a particular texture of a representation.
    // Server:
    // - The caching is done using vtkDistributedTrivalProducer::SetGlobalOutput(key, dobj).
    // Client:
    // - The client can reference datasets and images in the output of a trivial producer using
    // vtkDistributedTrivialProducer::UpdateFromGlobal(key).

    // Traverse the actors to cache datasets and images.
    // Instead of creating a unique key for each actor, it is created for each unique data object.
    const std::vector<std::string> queries = { this->NodeSelectors.begin(),
      this->NodeSelectors.end() };
    if (auto hierarchy = this->Importer->GetSceneHierarchy())
    {
      vtkNew<SceneNodeSerializer> visitor;
      visitor->Initialize(this);
      for (const auto& nodeId : hierarchy->SelectNodes(queries))
      {
        hierarchy->Visit(nodeId, visitor);
      }
      return visitor->ActorsJson;
    }
    return {};
  }

  //----------------------------------------------------------------------------
  void Serialize()
  {
    auto renderer = this->Importer->GetRenderer();
    auto props = renderer->GetViewProps();
    if (!props->GetNumberOfItems())
    {
      this->SceneDescription = nullptr;
      return;
    }
    using OutputAdapterType = ::output_vtk_buffer_adapter<PayloadType::ValueType>;
    using CBORWriter = nlohmann::detail::binary_writer<nlohmann::json, PayloadType::ValueType>;
    this->SceneDescription.TakeReference(PayloadType::New());
    auto adapter = std::make_shared<OutputAdapterType>(this->SceneDescription);
    CBORWriter writer(adapter);
    // serializes renderer into json cbor.
    // clang-format off
    writer.write_cbor(
      {
        { "Actors", this->SerializePropCollection() },
        { "Camera", to_json(renderer->GetActiveCamera()) },
        { "Lights", to_json(renderer->GetLights()) }
      });
    // clang-format on
  }

  //----------------------------------------------------------------------------
  std::string CacheDataObject(
    vtkDataObject* dobj, const std::string& userName, std::map<vtkDataObject*, std::string>& cache)
  {
    if (dobj == nullptr)
    {
      return "";
    }
    auto iter = cache.find(dobj);
    if (iter == cache.end())
    {
      // Create new cache item.
      const std::string key = userName;
      cache.insert(std::make_pair(dobj, key));
      this->DataObjectCacheKeys.emplace_back(key);
      vtkDistributedTrivialProducer::SetGlobalOutput(key.c_str(), dobj);
      return key;
    }
    else
    {
      const std::string& key = iter->second;
      return key;
    }
  }

  //----------------------------------------------------------------------------
  void ReleaseCache()
  {
    // Release previous cache entries.
    for (const auto& key : this->DataObjectCacheKeys)
    {
      vtkDistributedTrivialProducer::ReleaseGlobalOutput(key.c_str());
    }
    this->DataObjectCacheKeys.clear();
  }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMetaImporter);

//----------------------------------------------------------------------------
vtkMetaImporter::vtkMetaImporter()
  : Internals(new vtkInternals())
{
  this->SetActiveAssembly("DefaultScene");
}

//----------------------------------------------------------------------------
vtkMetaImporter::~vtkMetaImporter()
{
  auto& internals = (*this->Internals);
  internals.ReleaseCache();
  this->SetActiveAssembly(nullptr);
}

//----------------------------------------------------------------------------
void vtkMetaImporter::SetFileName(const char* filename)
{
  auto& internals = (*this->Internals);
  if (vtksys::SystemTools::StringEndsWith(filename, ".glb") ||
    vtksys::SystemTools::StringEndsWith(filename, ".gltf"))
  {
    auto gltfImporter = vtkGLTFImporter::New();
    gltfImporter->SetFileName(filename);
    internals.Importer.TakeReference(gltfImporter);
  }
  else if (vtksys::SystemTools::StringEndsWith(filename, ".3ds"))
  {
    auto threeDSImporter = vtk3DSImporter::New();
    threeDSImporter->SetFileName(filename);
    internals.Importer.TakeReference(threeDSImporter);
  }
  else if (vtksys::SystemTools::StringEndsWith(filename, ".obj"))
  {
    auto objImporter = vtkOBJImporter::New();
    objImporter->SetFileName(filename);
    internals.Importer.TakeReference(objImporter);
  }
  if (internals.ProgressEventObserverTag > 0)
  {
    internals.Importer->RemoveObserver(internals.ProgressEventObserverTag);
  }

  vtkNew<vtkEventForwarderCommand> forwarder;
  forwarder->SetTarget(this);
  internals.ProgressEventObserverTag =
    internals.Importer->AddObserver(vtkCommand::ProgressEvent, forwarder);
  this->FileName = filename;
}

//----------------------------------------------------------------------------
const char* vtkMetaImporter::GetFileName()
{
  auto& internals = (*this->Internals);
  if (internals.Importer == nullptr)
  {
    return nullptr;
  }
  else
  {
    return this->FileName.c_str();
  }
}

//----------------------------------------------------------------------------
void vtkMetaImporter::UpdateInformation()
{
  auto& internals = (*this->Internals);
  internals.ReleaseCache();
  if (internals.Importer == nullptr)
  {
    internals.SceneDescription = nullptr;
    this->AssemblyTag = 0;
  }
  else
  {
    internals.Importer->Update();
    internals.Serialize();
    this->AssemblyTag = internals.Importer->GetSceneHierarchy()->GetMTime();
  }
}

//----------------------------------------------------------------------------
vtkVariant vtkMetaImporter::GetSerializedSceneDescription()
{
  auto& internals = (*this->Internals);
  internals.ReleaseCache();
  internals.Serialize();
  return vtkVariant(internals.SceneDescription.Get());
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkMetaImporter::GetAssembly()
{
  auto& internals = (*this->Internals);
  if (internals.Importer == nullptr)
  {
    return nullptr;
  }
  return internals.Importer->GetSceneHierarchy();
}

//----------------------------------------------------------------------------
void vtkMetaImporter::AddNodeSelector(const char* selector)
{
  auto& internals = (*this->Internals);
  if (selector != nullptr)
  {
    internals.NodeSelectors.insert(selector);
  }
}

//----------------------------------------------------------------------------
void vtkMetaImporter::RemoveAllNodeSelectors()
{
  auto& internals = (*this->Internals);
  if (!internals.NodeSelectors.empty())
  {
    internals.NodeSelectors.clear();
  }
}

//----------------------------------------------------------------------------
void vtkMetaImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  auto& internals = (*this->Internals);
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Importer: \n";
  if (internals.Importer)
  {
    internals.Importer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(null)\n";
  }
}
