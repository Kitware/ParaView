// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMAnimationSceneSeriesWriter.h"

#include "vtkObjectFactory.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMViewProxy.h"
#include "vtkStringFormatter.h"

#include "vtksys/SystemTools.hxx"

vtkStandardNewMacro(vtkSMAnimationSceneSeriesWriter);

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneSeriesWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneSeriesWriter::SaveInitialize(int vtkNotUsed(startCount))
{
  if (!this->GetFrameExporterDelegate())
  {
    return false;
  }
  this->FrameCounter = this->StartFileCount;
  if (this->AnimationEnabled())
  {
    this->GetAnimationScene()->SetOverrideStillRender(true);
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneSeriesWriter::SaveFrame(double vtkNotUsed(time))
{
  auto fileNameProp = vtkSMPropertyHelper(this->GetFrameExporterDelegate(), "FileName");
  fileNameProp.Set(this->BuildCurrentFilePath().c_str());

  this->GetFrameExporterDelegate()->UpdateVTKObjects();
  this->GetFrameExporterDelegate()->GetView()->Update();
  this->GetFrameExporterDelegate()->Write();
  this->FrameCounter += this->Stride;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneSeriesWriter::SaveFinalize()
{
  if (this->AnimationEnabled())
  {
    this->GetAnimationScene()->SetOverrideStillRender(false);
  }
  return true;
}

//-----------------------------------------------------------------------------
std::string vtkSMAnimationSceneSeriesWriter::BuildCurrentFilePath()
{
  std::string filename = this->GetFileName();
  if (this->AnimationEnabled())
  {
    auto path = vtksys::SystemTools::GetFilenamePath(filename);
    auto prefix = vtksys::SystemTools::GetFilenameWithoutLastExtension(filename);
    auto ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
    filename = path + '/' + prefix;
    filename += std::string(".") + vtk::to_string(this->FrameCounter);
    filename += ext;
  }

  return filename;
}
