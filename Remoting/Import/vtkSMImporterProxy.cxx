// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMImporterProxy.h"
#include "vtkClientServerStream.h"
#include "vtkDataArrayRange.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMLightProxy.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

// clang-format off
#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

// Uncomment below line to dump client server stream logs to 'vtkSMImporterProxy.log'
// #define vtkSMImporterProxy_DEBUG_CLIENT_SERVER_STREAM

class vtkSMImporterProxy::vtkInternals
{
public:
  using PayloadType = vtkAOSDataArrayTemplate<vtkTypeUInt8>;

  std::vector<std::string> FileExtensions;
  vtkSmartPointer<PayloadType> Payload;

  const std::vector<std::string> VTKTextureNames = { "albedoTex", "materialTex", "anisotropyTex",
    "normalTex", "emissiveTex", "coatNormalTex" };
  const std::vector<std::string> ParaViewTextureNames = { "BaseColorTexture", "MaterialTexture",
    "AnisotropyTexture", "NormalTexture", "EmissiveTexture", "CoatNormalTexture" };

  /**
   * Reads the file on server and gets a json description of the scene.
   * @note
   *  The json description is transmitted by the server in the form of CBOR instead
   *  of an ASCII string. Caller is responsible for deserializing back to actual JSON datastructure.
   *  Useful links -
   *  1. https://cbor.io/
   *  2. https://nlohmann.github.io/json/api/basic_json/from_cbor/
   *  3. https://nlohmann.github.io/json/api/basic_json/to_cbor/
   */
  void FetchImporterPayload(vtkSMImporterProxy* importer)
  {
    vtkLogScopeFunction(TRACE);
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(importer)
           << "GetSerializedSceneDescription" << vtkClientServerStream::End;

#ifdef vtkSMImporterProxy_DEBUG_CLIENT_SERVER_STREAM
    auto session = importer->GetSession();
    session->GetSessionCore()->GetInterpreter()->SetLogFile("vtkSMImporterProxy.log");
#endif
    importer->ExecuteStream(stream);
#ifdef vtkSMImporterProxy_DEBUG_CLIENT_SERVER_STREAM
    session->GetSessionCore()->GetInterpreter()->SetLogFile(nullptr);
#endif

    this->Payload = nullptr;
    const auto lastResult = importer->GetLastResult();
    if (lastResult.GetCommand(0) == vtkClientServerStream::EndOfCommands)
    {
      vtkErrorWithObjectMacro(importer, "Unknown error occurred in client server stream.");
    }
    else if (lastResult.GetCommand(0) == vtkClientServerStream::Error)
    {
      const char* msg;
      lastResult.GetArgument(0, 0, &msg);
      vtkErrorWithObjectMacro(importer, "Failed to read file. Error message - " << msg);
    }
    else if (lastResult.GetCommand(0) == vtkClientServerStream::Reply)
    {
      int argument = 0;
      vtkVariant variant;
      lastResult.GetArgument(0, argument, &variant);
      if (!variant.IsValid())
      {
        vtkErrorWithObjectMacro(
          importer, "vtkSIImporterProxy::GetSerializedSceneDescription returned invalid variant!");
      }
      if (!variant.IsArray())
      {
        vtkErrorWithObjectMacro(importer,
          "vtkSIImporterProxy::GetSerializedSceneDescription did not return a vtkAbstractArray!");
      }
      if (auto abstractArray = variant.ToArray())
      {
        this->Payload = PayloadType::FastDownCast(abstractArray);
      }
    }
  }

  //----------------------------------------------------------------------------
  void ImportDataSources(vtkSMImporterProxy* importer, const nlohmann::json& actor) const
  {
    vtkLogScopeFunction(TRACE);
    const auto actorName = actor["Name"].get<std::string>();
    auto pxm = importer->GetSessionProxyManager();
    if (pxm->GetProxy(actorName.c_str()))
    {
      vtkWarningWithObjectMacro(importer, << "Found exisiting mesh producer " << actorName);
      return;
    }
    // create mesh.
    if (auto meshProducer = vtk::TakeSmartPointer(
          vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "DistributedTrivialProducer"))))
    {
      vtkNew<vtkSMParaViewPipelineController> controller;
      controller->PreInitializeProxy(meshProducer);
      const auto dobjKey = actor["InputDataObjectKey"].get<std::string>();
      vtkSMPropertyHelper(meshProducer, "UpdateDataset").Set(dobjKey.c_str());
      controller->PostInitializeProxy(meshProducer);
      controller->RegisterPipelineProxy(meshProducer, actorName.c_str());
    }
    else
    {
      vtkErrorWithObjectMacro(importer, << "Failed to create proxy. "
                                        << "groupName="
                                        << "sources,"
                                        << "proxyName="
                                        << "DistributedTrivialProducer");
      return;
    }
  }

  //----------------------------------------------------------------------------
  void ImportActor(vtkSMImporterProxy* importer, const nlohmann::json& actor,
    vtkSMRenderViewProxy* renderView) const
  {
    vtkLogScopeFunction(TRACE);
    // create display.
    const auto actorName = actor["Name"].get<std::string>();
    auto pxm = importer->GetSessionProxyManager();
    auto meshProducer = vtkSMSourceProxy::SafeDownCast(pxm->GetProxy(actorName.c_str()));
    if (!meshProducer)
    {
      vtkErrorWithObjectMacro(importer, << "Failed to find a mesh producer for " << actorName);
      return;
    }
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    auto repr =
      vtkSMRepresentationProxy::SafeDownCast(controller->Show(meshProducer, 0, renderView));
    if (!repr)
    {
      vtkErrorWithObjectMacro(importer, << "Failed to create a representation for " << actorName);
      return;
    }
    // copy display properties from actor over to the representation.
    this->CopyProperties(actor, repr);
    // create textures if they exist.
    const auto& displayProperty = actor["DisplayProperty"];
    if (displayProperty.contains("Textures"))
    {
      assert(this->VTKTextureNames.size() == this->ParaViewTextureNames.size());
      for (std::size_t i = 0; i < this->ParaViewTextureNames.size(); ++i)
      {
        if (auto texture =
              this->ImportTexture(importer, displayProperty, this->VTKTextureNames[i], actorName))
        {
          vtkSMPropertyHelper(repr, this->ParaViewTextureNames[i].c_str()).Set(texture);
        }
      }
    }
    repr->UpdateVTKObjects();
  }

  //----------------------------------------------------------------------------
  vtkSmartPointer<vtkSMProxy> ImportTexture(vtkSMImporterProxy* importer,
    const nlohmann::json& displayProperty, const std::string& textureName,
    const std::string& actorName) const
  {
    vtkLogScopeFunction(TRACE);
    if (!displayProperty.contains("Textures"))
    {
      return nullptr;
    }
    auto textures = displayProperty["Textures"];
    if (!textures[textureName].get<bool>())
    {
      return nullptr;
    }
    auto pxm = importer->GetSessionProxyManager();

    const auto regName = actorName + "_" + textureName;
    const auto tpKey = textures[textureName + "InputDataObjectKey"].get<std::string>();
    if (auto texture = vtk::TakeSmartPointer(pxm->NewProxy("textures", "ImageTexture")))
    {
      vtkNew<vtkSMParaViewPipelineController> controller;
      controller->PreInitializeProxy(texture);
      vtkSMPropertyHelper(texture, "Mode").Set("ReadFromMemory");
      vtkSMPropertyHelper(texture, "TrivialProducerKey").Set(tpKey.c_str());
      controller->PostInitializeProxy(texture);
      controller->RegisterTextureProxy(texture, tpKey.c_str(), regName.c_str());
      return texture;
    }
    else
    {
      vtkErrorWithObjectMacro(importer, << "Failed to create proxy. "
                                        << "groupName="
                                        << "textures,"
                                        << "proxyName="
                                        << "ImageTexture");
      return nullptr;
    }
  }

  //----------------------------------------------------------------------------
  void ImportLight(const nlohmann::json& light, vtkSMRenderViewProxy* view) const
  {
    // create light
    auto pxm = view->GetSessionProxyManager();
    auto lightProxy = vtk::TakeSmartPointer(
      vtkSMLightProxy::SafeDownCast(pxm->NewProxy("additional_lights", "Light")));
    vtkNew<vtkSMParaViewPipelineController> controller;
    controller->PreInitializeProxy(lightProxy);
    this->CopyProperties(light, lightProxy);
    controller->PostInitializeProxy(lightProxy);
    controller->RegisterLightProxy(lightProxy, view);
  }

  //----------------------------------------------------------------------------
  void ImportCamera(const nlohmann::json& camera, vtkSMRenderViewProxy* view) const
  {
    this->CopyProperties(camera, view);
    // For some reason, this property is exposed on the view instead of camera.
    this->AssignPropertyValue<vtkTypeBool>(
      "ParallelProjection", camera, view, "CameraParallelProjection");
  }

private:
  /**
   * Copies the value from the json to the given property on the proxy.
   */
  template <typename T>
  static void AssignPropertyValue(const std::string& propName, const nlohmann::json& from,
    vtkSMProxy* proxy, std::string destPropName = "")
  {
    if (destPropName.empty())
    {
      destPropName = propName;
    }
    try
    {
      vtkSMPropertyHelper(proxy, destPropName.c_str()).Set(from.at(propName).get<T>());
    }
    catch (const nlohmann::json::exception& e)
    {
      vtkGenericWarningMacro("Failed to assign value for property "
        << destPropName << " on proxy " << proxy->GetObjectDescription() << "\n"
        << "Message: " << e.what() << '\n'
        << "Exception id: " << e.id << '\n');
    }
  }

  template <typename T, int N>
  static void AssignPropertyArray(const std::string& propName, const nlohmann::json& from,
    vtkSMProxy* proxy, std::string destPropName = "")
  {
    if (destPropName.empty())
    {
      destPropName = propName;
    }
    try
    {
      vtkSMPropertyHelper(proxy, destPropName.c_str())
        .Set(from.at(propName).get<std::vector<T>>().data(), N);
    }
    catch (const nlohmann::json::exception& e)
    {
      vtkGenericWarningMacro("Failed to assign array for property "
        << destPropName << " on proxy " << proxy->GetObjectDescription() << "\n"
        << "Message: " << e.what() << '\n'
        << "Exception id: " << e.id << '\n');
    }
  }

  /**
   * Copies properties from actor to the representation.
   */
  void CopyProperties(const nlohmann::json& actor, vtkSMRepresentationProxy* repr) const
  {
    // copy actor transformation to representation.
    this->AssignPropertyArray<double, 16>("UserTransform", actor, repr);
    this->AssignPropertyArray<double, 3>("Origin", actor, repr);
    this->AssignPropertyArray<double, 3>("Translation", actor, repr);
    this->AssignPropertyArray<double, 3>("Orientation", actor, repr);
    this->AssignPropertyArray<double, 3>("Scale", actor, repr);
    auto flipTextures = actor["FlipTextures"].get<bool>();
    if (flipTextures)
    {
      auto pxm = repr->GetSessionProxyManager();

      auto transformProxy = vtk::TakeSmartPointer(pxm->NewProxy("extended_sources", "Transform2"));
      vtkNew<vtkSMParaViewPipelineController> controller;
      controller->PreInitializeProxy(transformProxy);
      vtkSMPropertyHelper(transformProxy, "Scale").Set(1, -1);
      controller->PostInitializeProxy(transformProxy);
      pxm->RegisterProxy("extended_sources", transformProxy);
      vtkSMPropertyHelper(repr, "TextureTransform").Set(transformProxy);
    }
    // copy display properties over to representation.
    const auto displayProperty = actor["DisplayProperty"];
    this->AssignPropertyArray<double, 3>("AmbientColor", displayProperty, repr);
    this->AssignPropertyArray<double, 3>("DiffuseColor", displayProperty, repr);
    this->AssignPropertyArray<double, 3>("SpecularColor", displayProperty, repr);
    this->AssignPropertyArray<double, 3>("EdgeColor", displayProperty, repr);
    // this is fine, paraview representation omits the opacity for selection color
    this->AssignPropertyArray<double, 3>("SelectionColor", displayProperty, repr);
    this->AssignPropertyValue<double>("Ambient", displayProperty, repr);
    this->AssignPropertyValue<double>("Diffuse", displayProperty, repr);
    this->AssignPropertyValue<double>("Metallic", displayProperty, repr);
    this->AssignPropertyValue<double>("Roughness", displayProperty, repr);
    this->AssignPropertyValue<double>("Anisotropy", displayProperty, repr);
    this->AssignPropertyValue<double>("AnisotropyRotation", displayProperty, repr);
    this->AssignPropertyValue<double>("BaseIOR", displayProperty, repr);
    this->AssignPropertyValue<double>("CoatIOR", displayProperty, repr);
    this->AssignPropertyArray<double, 3>("CoatColor", displayProperty, repr);
    this->AssignPropertyValue<double>("CoatRoughness", displayProperty, repr);
    this->AssignPropertyValue<double>("CoatStrength", displayProperty, repr);
    this->AssignPropertyValue<double>("CoatNormalScale", displayProperty, repr);
    this->AssignPropertyValue<double>("NormalScale", displayProperty, repr);
    this->AssignPropertyValue<double>("OcclusionStrength", displayProperty, repr);
    this->AssignPropertyArray<double, 3>("EmissiveFactor", displayProperty, repr);
    this->AssignPropertyValue<double>("Specular", displayProperty, repr);
    this->AssignPropertyValue<double>("SpecularPower", displayProperty, repr);
    this->AssignPropertyValue<double>("Opacity", displayProperty, repr);
    this->AssignPropertyValue<double>("EdgeOpacity", displayProperty, repr);
    this->AssignPropertyArray<double, 3>("EdgeTint", displayProperty, repr);
    this->AssignPropertyValue<float>("PointSize", displayProperty, repr);
    this->AssignPropertyValue<float>("LineWidth", displayProperty, repr);
    this->AssignPropertyValue<float>("SelectionPointSize", displayProperty, repr);
    this->AssignPropertyValue<float>("SelectionLineWidth", displayProperty, repr);
    this->AssignPropertyValue<int>("Interpolation", displayProperty, repr);
    switch (displayProperty["Representation"].get<int>())
    {
      case 0:
        repr->SetRepresentationType("Points");
        break;
      case 1:
        repr->SetRepresentationType("Wireframe");
        break;
      case 2:
      default:
        if (displayProperty["EdgeVisibility"].get<vtkTypeBool>())
        {
          repr->SetRepresentationType("Surface With Edges");
        }
        else
        {
          repr->SetRepresentationType("Surface");
        }
        break;
    }
    this->AssignPropertyValue<bool>("RenderPointsAsSpheres", displayProperty, repr);
    this->AssignPropertyValue<bool>("RenderLinesAsTubes", displayProperty, repr);
    this->AssignPropertyValue<bool>("ShowTexturesOnBackface", displayProperty, repr);
  }

  /**
   * Copies properties from light to the light proxy.
   */
  void CopyProperties(const nlohmann::json& light, vtkSMLightProxy* lightProxy) const
  {
    this->AssignPropertyArray<double, 3>("FocalPoint", light, lightProxy);
    this->AssignPropertyArray<double, 3>("FocalPoint", light, lightProxy);
    this->AssignPropertyArray<double, 3>(
      "Position", light, lightProxy, /*destPropName=*/"LightPosition");
    this->AssignPropertyValue<double>(
      "Intensity", light, lightProxy, /*destPropName=*/"LightIntensity");
    this->AssignPropertyArray<double, 3>("DiffuseColor", light, lightProxy);
    this->AssignPropertyValue<vtkTypeBool>(
      "Switch", light, lightProxy, /*destPropName*/ "LightSwitch");
    this->AssignPropertyValue<vtkTypeBool>("Positional", light, lightProxy);
    this->AssignPropertyValue<double>("ConeAngle", light, lightProxy);
    this->AssignPropertyValue<int>("LightType", light, lightProxy);
  }

  /**
   * Copies properties from camera to the camera proxy.
   */
  void CopyProperties(const nlohmann::json& camera, vtkSMRenderViewProxy* view) const
  {
    this->AssignPropertyArray<double, 3>("Position", camera, view, "CameraPosition");
    this->AssignPropertyArray<double, 3>("FocalPoint", camera, view, "CameraFocalPoint");
    this->AssignPropertyArray<double, 3>("ViewUp", camera, view, "CameraViewUp");
    this->AssignPropertyValue<double>("ViewAngle", camera, view, "CameraViewAngle");
    this->AssignPropertyValue<double>("FocalDisk", camera, view, "CameraFocalDisk");
    this->AssignPropertyValue<double>("FocalDistance", camera, view, "CameraFocalDistance");
    this->AssignPropertyValue<double>("ParallelScale", camera, view, "CameraParallelScale");
    this->AssignPropertyValue<double>("EyeAngle", camera, view);
    this->AssignPropertyArray<double, 16>("EyeTransformMatrix", camera, view);
    this->AssignPropertyArray<double, 16>("ModelTransformMatrix", camera, view);
  }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMImporterProxy);

//----------------------------------------------------------------------------
vtkSMImporterProxy::vtkSMImporterProxy()
  : Internals(new vtkInternals())
{
  this->SetSIClassName("vtkSIImporterProxy");
}

//----------------------------------------------------------------------------
vtkSMImporterProxy::~vtkSMImporterProxy() = default;

//----------------------------------------------------------------------------
void vtkSMImporterProxy::UpdatePipelineInformation()
{
  if (this->ObjectsCreated)
  {
    // this could take a while because the server imports the file and generates metadata.
    // prepare to report progress.
    this->GetSession()->PrepareProgress();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << SIPROXY(this) << "UpdatePipelineInformation"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
    this->GetSession()->CleanupPendingProgress();
  }

  // This simply iterates over subproxies and calls UpdatePropertyInformation();
  this->Superclass::UpdatePipelineInformation();

  this->InvokeEvent(vtkCommand::UpdateInformationEvent);
}

//----------------------------------------------------------------------------
void vtkSMImporterProxy::Import(vtkSMRenderViewProxy* renderView)
{
  auto& internals = (*this->Internals);
  if (!this->ObjectsCreated)
  {
    return;
  }
  internals.FetchImporterPayload(this);
  if (!internals.Payload)
  {
    return;
  }
  if (!internals.Payload->GetNumberOfValues())
  {
    vtkWarningMacro(<< "Receieved empty scene description. Cannot import file "
                    << vtkSMPropertyHelper(this, "FileName").GetAsString());
    return;
  }
  // Deserialize scene description from CBOR.
  nlohmann::json scene;
  try
  {
    auto payloadRange = vtk::DataArrayValueRange(internals.Payload);
    scene = nlohmann::json::from_cbor(payloadRange.begin(), payloadRange.end());
  }
  catch (const nlohmann::json::parse_error& e)
  {
    vtkErrorMacro(<< "Failed to parse scene description from cbor.\n"
                  << "Message: " << e.what() << '\n'
                  << "Exception id: " << e.id << '\n'
                  << "Byte position of error: " << e.byte);
    return;
  }
  if (scene.empty())
  {
    vtkWarningMacro(<< "Receieved empty scene description. Cannot import file "
                    << vtkSMPropertyHelper(this, "FileName").GetAsString());
    return;
  }
  // Import actors
  if (scene.contains("Actors"))
  {
    for (const auto& actor : scene["Actors"])
    {
      const auto name = actor["Name"].get<std::string>();
      internals.ImportDataSources(this, actor);
      internals.ImportActor(this, actor, renderView);
    }
  }
  // Import camera
  if (scene.contains("Camera"))
  {
    internals.ImportCamera(scene["Camera"], renderView);
  }
  // Import lights
  if (scene.contains("Lights") && !scene["Lights"].empty())
  {
    // Turn off light kit because this scene has one or more custom lights.
    vtkSMPropertyHelper(renderView, "UseLight").Set(0);
    for (const auto& light : scene["Lights"])
    {
      const auto name = light["Name"].get<std::string>();
      internals.ImportLight(light, renderView);
    }
  }
  renderView->UpdateVTKObjects();
  renderView->Update();
}

//----------------------------------------------------------------------------
const std::vector<std::string>& vtkSMImporterProxy::GetFileExtensions() const
{
  return this->Internals->FileExtensions;
}

//----------------------------------------------------------------------------
int vtkSMImporterProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pxm, vtkPVXMLElement* element)
{
  // we let the superclass read in information first so that we can just
  // get the proper hints (i.e. not hints from base proxies) after that to
  // figure out file extensions
  const int retVal = this->Superclass::ReadXMLAttributes(pxm, element);
  if (auto hintElement = this->GetHints())
  {
    if (auto importerFactoryElement = hintElement->FindNestedElementByName("ImporterFactory"))
    {
      if (const char* exts = importerFactoryElement->GetAttribute("extensions"))
      {
        std::vector<std::string> extensionsVec;
        vtksys::SystemTools::Split(exts, extensionsVec, ' ');
        for (auto iter = extensionsVec.begin(); iter != extensionsVec.end(); iter++)
        {
          this->Internals->FileExtensions.push_back(*iter);
        }
      }
    }
  }
  return retVal;
}

//----------------------------------------------------------------------------
void vtkSMImporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "vtkSMImporterProxy::vtkInternals (" << this->Internals.get() << "):\n";
  os << indent << "FileExtensions:";
  for (const auto& fname : this->Internals->FileExtensions)
  {
    os << " " << fname;
  }
  os << '\n';
  os << indent << "Payload:";
  if (this->Internals->Payload)
  {
    os << "\n";
    this->Internals->Payload->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(null)\n";
  }
}
