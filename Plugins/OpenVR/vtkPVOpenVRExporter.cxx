/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOpenVRExporter.h"

#include "vtkCamera.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkEquirectangularToCubeMapTexture.h"
#include "vtkFlagpoleLabel.h"
#include "vtkJPEGWriter.h"
#include "vtkLight.h"
#include "vtkMapper.h"
#include "vtkNumberToString.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVROverlayInternal.h"
#include "vtkPVOpenVRHelper.h"
#include "vtkPVRenderView.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMViewProxy.h"
#include "vtkScalarsToColors.h"
#include "vtkShaderProperty.h"
#include "vtkTexture.h"
#include "vtkWindowToImageFilter.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataObjectWriter.h"
#include "vtkXMLUtilities.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"
#include <QCoreApplication>

vtkStandardNewMacro(vtkPVOpenVRExporter);

void vtkPVOpenVRExporter::ExportLocationsAsSkyboxes(vtkPVOpenVRHelper* helper,
  vtkSMViewProxy* smview, std::map<int, vtkPVOpenVRHelperLocation>& locations)
{
  auto* view = vtkPVRenderView::SafeDownCast(smview->GetClientSideView());
  vtkRenderer* pvRenderer = view->GetRenderView()->GetRenderer();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(1024, 1024);
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(pvRenderer->GetBackground());
  renWin->AddRenderer(ren);
  ren->SetClippingRangeExpansion(0.05);

  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(0.0, 1.0, 0.0);
    light->SetIntensity(1.0);
    light->SetLightTypeToSceneLight();
    ren->AddLight(light);
    light->Delete();
  }
  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(0.8, -0.2, 0.0);
    light->SetIntensity(0.8);
    light->SetLightTypeToSceneLight();
    ren->AddLight(light);
    light->Delete();
  }
  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(-0.3, -0.2, 0.7);
    light->SetIntensity(0.6);
    light->SetLightTypeToSceneLight();
    ren->AddLight(light);
    light->Delete();
  }
  {
    vtkLight* light = vtkLight::New();
    light->SetPosition(-0.3, -0.2, -0.7);
    light->SetIntensity(0.4);
    light->SetLightTypeToSceneLight();
    ren->AddLight(light);
    light->Delete();
  }

  std::string dir = "pv-skybox/";
  vtksys::SystemTools::MakeDirectory(dir);
  vtksys::ofstream json("pv-skybox/index.json");
  json << "{ \"data\": [ { \"mimeType\": \"image/jpg\","
          "\"pattern\": \"{poseIndex}/{orientation}.jpg\","
          "\"type\": \"blob\", \"name\": \"image\", \"metadata\": {}}], "
          "\"type\": [\"tonic-query-data-model\"],"
          "\"arguments\": { \"poseIndex\": { \"values\": [";

  int count = 0;
  for (auto& loci : locations)
  {
    auto& loc = loci.second;
    if (!loc.Pose->Loaded)
    {
      continue;
    }
    // create subdir for each pose
    std::ostringstream sdir;
    sdir << dir << count;
    vtksys::SystemTools::MakeDirectory(sdir.str());

    helper->LoadLocationState(loci.first);

    auto& camPose = *loc.Pose;

    //    QCoreApplication::processEvents();
    //  this->SMView->StillRender();

    renWin->Render();

    double vright[3];

    vtkMath::Cross(camPose.PhysicalViewDirection, camPose.PhysicalViewUp, vright);

    // now generate the six images, right, left, top, bottom, back, front
    //
    double directions[6][3] = { vright[0], vright[1], vright[2], -1 * vright[0], -1 * vright[1],
      -1 * vright[2], camPose.PhysicalViewUp[0], camPose.PhysicalViewUp[1],
      camPose.PhysicalViewUp[2], -1 * camPose.PhysicalViewUp[0], -1 * camPose.PhysicalViewUp[1],
      -1 * camPose.PhysicalViewUp[2], -1 * camPose.PhysicalViewDirection[0],
      -1 * camPose.PhysicalViewDirection[1], -1 * camPose.PhysicalViewDirection[2],
      camPose.PhysicalViewDirection[0], camPose.PhysicalViewDirection[1],
      camPose.PhysicalViewDirection[2] };
    double vups[6][3] = { camPose.PhysicalViewUp[0], camPose.PhysicalViewUp[1],
      camPose.PhysicalViewUp[2], camPose.PhysicalViewUp[0], camPose.PhysicalViewUp[1],
      camPose.PhysicalViewUp[2], -1 * camPose.PhysicalViewDirection[0],
      -1 * camPose.PhysicalViewDirection[1], -1 * camPose.PhysicalViewDirection[2],
      camPose.PhysicalViewDirection[0], camPose.PhysicalViewDirection[1],
      camPose.PhysicalViewDirection[2], camPose.PhysicalViewUp[0], camPose.PhysicalViewUp[1],
      camPose.PhysicalViewUp[2], camPose.PhysicalViewUp[0], camPose.PhysicalViewUp[1],
      camPose.PhysicalViewUp[2] };

    const char* dirnames[6] = { "right", "left", "up", "down", "back", "front" };

    if (count)
    {
      json << ",";
    }
    json << " \"" << count << "\"";

    vtkCamera* cam = ren->GetActiveCamera();
    cam->SetViewAngle(90);
    cam->SetPosition(camPose.Position);
    // doubel *drange = ren->GetActiveCamera()->GetClippingRange();
    // cam->SetClippingRange(0.2*camPose.Distance, drange);
    double* pos = cam->GetPosition();

    renWin->MakeCurrent();
    // remove prior props
    vtkCollectionSimpleIterator pit;
    ren->RemoveAllViewProps();

    vtkActorCollection* acol = pvRenderer->GetActors();
    vtkActor* actor;
    for (acol->InitTraversal(pit); (actor = acol->GetNextActor(pit));)
    {
      actor->ReleaseGraphicsResources(pvRenderer->GetVTKWindow());
      ren->AddActor(actor);
    }

    for (int j = 0; j < 6; ++j)
    {
      // view angle of 90
      cam->SetFocalPoint(pos[0] + camPose.Distance * directions[j][0],
        pos[1] + camPose.Distance * directions[j][1], pos[2] + camPose.Distance * directions[j][2]);
      cam->SetViewUp(vups[j][0], vups[j][1], vups[j][2]);
      ren->ResetCameraClippingRange();
      double* crange = cam->GetClippingRange();
      cam->SetClippingRange(0.2 * camPose.Distance, crange[1]);

      vtkNew<vtkWindowToImageFilter> w2i;
      w2i->SetInput(renWin);
      w2i->SetInputBufferTypeToRGB();
      w2i->ReadFrontBufferOff(); // read from the back buffer
      w2i->Update();

      vtkNew<vtkJPEGWriter> jwriter;
      std::ostringstream filename;
      filename << sdir.str();
      filename << "/" << dirnames[j] << ".jpg";
      jwriter->SetFileName(filename.str().c_str());
      jwriter->SetQuality(75);
      jwriter->SetInputConnection(w2i->GetOutputPort());
      jwriter->Write();
    }
    count++;
  }

  json << "], \"name\": \"poseIndex\" }, "
          "\"orientation\": { \"values\": [\"right\", \"left\", \"up\", "
          "\"down\", \"back\", "
          "\"front\"], "
          "  \"name\": \"orientation\" } }, "
          "\"arguments_order\": [\"orientation\", \"poseIndex\"], "
          "\"metadata\": {\"backgroundColor\": \"rgb(0, 0, 0)\"} }";
}

namespace
{
vtkPolyData* findPolyData(vtkDataObject* input)
{
  // do we have polydata?
  vtkPolyData* pd = vtkPolyData::SafeDownCast(input);
  if (pd)
  {
    return pd;
  }
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(input);
  if (cd)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
      {
        return pd;
      }
    }
  }
  return nullptr;
}
}

namespace
{
template <typename T>
void setVectorAttribute(vtkXMLDataElement* el, const char* name, int count, T* data)
{
  std::ostringstream o;
  vtkNumberToString convert;
  for (int i = 0; i < count; ++i)
  {
    if (i)
    {
      o << " ";
    }
    o << convert(data[i]);
  }
  el->SetAttribute(name, o.str().c_str());
}

void writeTextureReference(vtkXMLDataElement* adatael, vtkTexture* texture, const char* tname,
  std::map<vtkTexture*, size_t>& textures)
{
  if (texture)
  {
    adatael->SetIntAttribute(tname, static_cast<int>(textures[texture]));
  }
}
}

void vtkPVOpenVRExporter::ExportLocationsAsView(vtkPVOpenVRHelper* helper, vtkSMViewProxy* smview,
  std::map<int, vtkPVOpenVRHelperLocation>& locations)
{
  auto* view = vtkPVRenderView::SafeDownCast(smview->GetClientSideView());
  vtkRenderer* pvRenderer = view->GetRenderView()->GetRenderer();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(1024, 1024);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetClippingRangeExpansion(0.05);

  std::string dir = "pv-view/";
  vtksys::SystemTools::MakeDirectory(dir);

  std::map<vtkActor*, size_t> actors;
  std::map<vtkDataObject*, size_t> dataobjects;
  std::map<vtkTexture*, size_t> textures;

  vtkNew<vtkXMLDataElement> topel;
  topel->SetName("View");
  topel->SetIntAttribute("Version", 2);
  topel->SetIntAttribute("UseImageBasedLighting", pvRenderer->GetUseImageBasedLighting());
  if (pvRenderer->GetEnvironmentTexture())
  {
    vtkTexture* cubetex = pvRenderer->GetEnvironmentTexture();
    if (textures.find(cubetex) == textures.end())
    {
      textures[cubetex] = textures.size();
    }
    topel->SetIntAttribute("EnvironmentTexture", static_cast<int>(textures[cubetex]));
  }

  vtkNew<vtkXMLDataElement> posesel;
  posesel->SetName("CameraPoses");
  int count = 0;
  for (auto& loci : locations)
  {
    auto& loc = loci.second;
    if (!loc.Pose->Loaded)
    {
      continue;
    }

    vtkOpenVRCameraPose& pose = *loc.Pose;

    helper->LoadLocationState(loci.first);

    QCoreApplication::processEvents();
    smview->StillRender();

    vtkNew<vtkXMLDataElement> poseel;
    poseel->SetName("CameraPose");
    poseel->SetIntAttribute("PoseNumber", static_cast<int>(count + 1));
    setVectorAttribute(poseel, "Position", 3, pose.Position);
    poseel->SetDoubleAttribute("Distance", pose.Distance);
    poseel->SetDoubleAttribute("MotionFactor", pose.MotionFactor);
    setVectorAttribute(poseel, "Translation", 3, pose.Translation);
    setVectorAttribute(poseel, "InitialViewUp", 3, pose.PhysicalViewUp);
    setVectorAttribute(poseel, "InitialViewDirection", 3, pose.PhysicalViewDirection);
    setVectorAttribute(poseel, "ViewDirection", 3, pose.ViewDirection);

    vtkCollectionSimpleIterator pit;
    vtkActorCollection* acol = pvRenderer->GetActors();
    vtkActor* actor;
    for (acol->InitTraversal(pit); (actor = acol->GetNextActor(pit));)
    {
      vtkNew<vtkXMLDataElement> actorel;
      actorel->SetName("Actor");

      if (!actor->GetVisibility() || !actor->GetMapper() ||
        !actor->GetMapper()->GetInputAlgorithm())
      {
        continue;
      }

      // handle flagpoles in the extra file
      if (vtkFlagpoleLabel::SafeDownCast(actor))
      {
        continue;
      }

      // get the polydata
      actor->GetMapper()->GetInputAlgorithm()->Update();
      vtkPolyData* pd = findPolyData(actor->GetMapper()->GetInputDataObject(0, 0));
      if (!pd || pd->GetNumberOfCells() == 0)
      {
        continue;
      }

      if (actors.find(actor) == actors.end())
      {
        actors[actor] = actors.size();
      }

      actorel->SetIntAttribute("ActorID", static_cast<int>(actors[actor]));
      poseel->AddNestedElement(actorel);
    }

    posesel->AddNestedElement(poseel);
    count++;
  }
  topel->AddNestedElement(posesel);

  // export the actors
  vtkNew<vtkXMLDataElement> actorsel;
  actorsel->SetName("Actors");
  for (auto& ait : actors)
  {
    auto& actor = ait.first;

    // record the textures if there are any
    if (actor->GetTexture() && actor->GetTexture()->GetInput())
    {
      vtkTexture* texture = actor->GetTexture();
      if (textures.find(texture) == textures.end())
      {
        textures[texture] = textures.size();
      }
    }

    // record the property textures if any
    std::map<std::string, vtkTexture*>& tex = actor->GetProperty()->GetAllTextures();
    for (auto& t : tex)
    {
      vtkTexture* texture = t.second;
      if (textures.find(texture) == textures.end())
      {
        textures[texture] = textures.size();
      }
    }

    // record actor properties
    vtkNew<vtkXMLDataElement> adatael;
    adatael->SetName("ActorData");

    // write out texture references
    writeTextureReference(adatael, actor->GetTexture(), "TextureID", textures);
    for (auto& t : tex)
    {
      writeTextureReference(adatael, t.second, t.first.c_str(), textures);
    }

    vtkPolyData* pd = findPolyData(actor->GetMapper()->GetInputDataObject(0, 0));
    // record the polydata if not already done
    if (dataobjects.find(pd) == dataobjects.end())
    {
      dataobjects[pd] = dataobjects.size();
    }
    adatael->SetIntAttribute("PolyDataID", static_cast<int>(dataobjects[pd]));

    adatael->SetIntAttribute("ActorID", static_cast<int>(ait.second));
    adatael->SetAttribute("ClassName", actor->GetClassName());
    adatael->SetVectorAttribute("DiffuseColor", 3, actor->GetProperty()->GetDiffuseColor());
    adatael->SetDoubleAttribute("Diffuse", actor->GetProperty()->GetDiffuse());
    adatael->SetVectorAttribute("AmbientColor", 3, actor->GetProperty()->GetAmbientColor());
    adatael->SetDoubleAttribute("Ambient", actor->GetProperty()->GetAmbient());
    adatael->SetVectorAttribute("SpecularColor", 3, actor->GetProperty()->GetSpecularColor());
    adatael->SetDoubleAttribute("Specular", actor->GetProperty()->GetSpecular());
    adatael->SetDoubleAttribute("SpecularPower", actor->GetProperty()->GetSpecularPower());
    adatael->SetDoubleAttribute("Opacity", actor->GetProperty()->GetOpacity());
    adatael->SetDoubleAttribute("LineWidth", actor->GetProperty()->GetLineWidth());

    adatael->SetIntAttribute("Interpolation", actor->GetProperty()->GetInterpolation());
    adatael->SetDoubleAttribute("Metallic", actor->GetProperty()->GetMetallic());
    adatael->SetDoubleAttribute("Roughness", actor->GetProperty()->GetRoughness());
    adatael->SetDoubleAttribute("NormalScale", actor->GetProperty()->GetNormalScale());
    adatael->SetDoubleAttribute("OcclusionStrength", actor->GetProperty()->GetOcclusionStrength());
    adatael->SetVectorAttribute("EmissiveFactor", 3, actor->GetProperty()->GetEmissiveFactor());

    adatael->SetVectorAttribute("Scale", 3, actor->GetScale());
    setVectorAttribute(adatael, "Position", 3, actor->GetPosition());
    setVectorAttribute(adatael, "Origin", 3, actor->GetOrigin());
    setVectorAttribute(adatael, "Orientation", 3, actor->GetOrientation());

    // scalar visibility
    adatael->SetIntAttribute("ScalarVisibility", actor->GetMapper()->GetScalarVisibility());
    adatael->SetIntAttribute("ScalarMode", actor->GetMapper()->GetScalarMode());

    // shader replacements
    auto shaderp = actor->GetShaderProperty();
    if (shaderp && shaderp->GetNumberOfShaderReplacements())
    {
      // export the actors
      vtkNew<vtkXMLDataElement> shadersel;
      shadersel->SetName("Shaders");
      int num = shaderp->GetNumberOfShaderReplacements();
      for (int i = 0; i < num; ++i)
      {
        vtkNew<vtkXMLDataElement> shaderel;
        shaderel->SetName("Shader");
        shaderel->SetAttribute("Type", shaderp->GetNthShaderReplacementTypeAsString(i).c_str());
        std::string name;
        std::string repValue;
        bool first;
        bool all;
        shaderp->GetNthShaderReplacement(i, name, first, repValue, all);
        shaderel->SetAttribute("Name", name.c_str());
        shaderel->SetAttribute("Replacement", repValue.c_str());
        shaderel->SetIntAttribute("First", first ? 1 : 0);
        shaderel->SetIntAttribute("All", all ? 1 : 0);
        shadersel->AddNestedElement(shaderel);
      }
      adatael->AddNestedElement(shadersel);
    }

    if (actor->GetMapper()->GetUseLookupTableScalarRange())
    {
      adatael->SetVectorAttribute(
        "ScalarRange", 2, actor->GetMapper()->GetLookupTable()->GetRange());
    }
    else
    {
      setVectorAttribute(adatael, "ScalarRange", 2, actor->GetMapper()->GetScalarRange());
    }
    adatael->SetIntAttribute("ScalarArrayId", actor->GetMapper()->GetArrayId());
    adatael->SetIntAttribute("ScalarArrayAccessMode", actor->GetMapper()->GetArrayAccessMode());
    adatael->SetIntAttribute("ScalarArrayComponent", actor->GetMapper()->GetArrayComponent());
    adatael->SetAttribute("ScalarArrayName", actor->GetMapper()->GetArrayName());

    actorsel->AddNestedElement(adatael);
  }
  topel->AddNestedElement(actorsel);

  // now write the textures
  vtkNew<vtkXMLDataElement> texturesel;
  texturesel->SetName("Textures");
  for (auto const& t : textures)
  {
    vtkNew<vtkXMLDataElement> texel;
    texel->SetName("Texture");
    texel->SetAttribute("ClassName", t.first->GetClassName());
    texel->SetIntAttribute("Repeat", t.first->GetRepeat());
    texel->SetIntAttribute("Interpolate", t.first->GetInterpolate());
    texel->SetIntAttribute("Mipmap", t.first->GetMipmap());
    texel->SetIntAttribute("UseSRGBColorSpace", t.first->GetUseSRGBColorSpace() ? 1 : 0);
    texel->SetDoubleAttribute(
      "MaximumAnisotropicFiltering", t.first->GetMaximumAnisotropicFiltering());
    texel->SetIntAttribute("TextureID", static_cast<int>(t.second));

    vtkImageData* idata = nullptr;
    if (t.first->GetCubeMap())
    {
      vtkEquirectangularToCubeMapTexture* cubetex =
        vtkEquirectangularToCubeMapTexture::SafeDownCast(t.first);
      if (cubetex)
      {
        cubetex->GetInputTexture()->Update();
        idata = cubetex->GetInputTexture()->GetInput();
      }
    }
    else
    {
      t.first->Update();
      idata = vtkImageData::SafeDownCast(t.first->GetInputDataObject(0, 0));
    }

    if (dataobjects.find(idata) == dataobjects.end())
    {
      dataobjects[idata] = dataobjects.size();
    }

    texel->SetIntAttribute("ImageDataID", static_cast<int>(dataobjects[idata]));
    texturesel->AddNestedElement(texel);
  }
  topel->AddNestedElement(texturesel);

  // create subdir for the data
  std::string datadir = dir;
  datadir += "data/";
  vtksys::SystemTools::MakeDirectory(datadir);

  // now write the dataobjects
  vtkNew<vtkXMLDataElement> datasel;
  datasel->SetName("DataObjects");
  for (auto const& dit : dataobjects)
  {
    vtkDataObject* data = dit.first;
    vtkNew<vtkXMLDataElement> datael;
    datael->SetName("DataObject");
    datael->SetIntAttribute("DataObjectType", data->GetDataObjectType());
    datael->SetIntAttribute("MemorySize", data->GetActualMemorySize());
    datael->SetIntAttribute("DataObjectID", static_cast<int>(dit.second));
    datasel->AddNestedElement(datael);

    std::ostringstream sdir;
    sdir << "data" << dit.second;
    switch (data->GetDataObjectType())
    {
      case VTK_IMAGE_DATA:
        sdir << ".vti";
        break;
      case VTK_POLY_DATA:
        sdir << ".vtp";
        break;
    }
    std::string fileName = "data/" + sdir.str();
    datael->SetAttribute("FileName", fileName.c_str());
    vtkNew<vtkXMLDataObjectWriter> writer;
    writer->SetDataModeToAppended();
    writer->SetCompressorTypeToLZ4();
    writer->EncodeAppendedDataOff();
    fileName = datadir + sdir.str();
    writer->SetFileName(fileName.c_str());
    writer->SetInputData(data);
    writer->Write();
  }
  topel->AddNestedElement(datasel);

  vtkIndent indent;
  vtkXMLUtilities::WriteElementToFile(topel, "pv-view/index.mvx", &indent);

  // create empty extra.xml file
  vtkNew<vtkXMLDataElement> topel2;
  topel2->SetName("View");
  topel2->SetAttribute("ViewImage", "Filename.jpg");
  topel2->SetAttribute("Longitude", "0.0");
  topel2->SetAttribute("Latitude", "0.0");

  // write out flagpoles
  vtkNew<vtkXMLDataElement> fsel;
  fsel->SetName("Flagpoles");
  vtkCollectionSimpleIterator pit;
  vtkActorCollection* acol = pvRenderer->GetActors();
  vtkActor* actor;
  for (acol->InitTraversal(pit); (actor = acol->GetNextActor(pit));)
  {
    vtkFlagpoleLabel* flag = vtkFlagpoleLabel::SafeDownCast(actor);

    if (!flag || !actor->GetVisibility())
    {
      continue;
    }

    vtkNew<vtkXMLDataElement> flagel;
    flagel->SetName("Flagpole");
    flagel->SetAttribute("Label", flag->GetInput());
    flagel->SetVectorAttribute("Position", 3, flag->GetBasePosition());
    flagel->SetDoubleAttribute("Height",
      sqrt(vtkMath::Distance2BetweenPoints(flag->GetTopPosition(), flag->GetBasePosition())));

    fsel->AddNestedElement(flagel);
  }
  topel2->AddNestedElement(fsel);

  vtkNew<vtkXMLDataElement> psel;
  psel->SetName("PhotoSpheres");
  vtkNew<vtkXMLDataElement> cpel;
  cpel->SetName("CameraPoses");
  topel2->AddNestedElement(psel);
  topel2->AddNestedElement(cpel);
  vtkXMLUtilities::WriteElementToFile(topel2, "pv-view/extra.xml", &indent);
}
