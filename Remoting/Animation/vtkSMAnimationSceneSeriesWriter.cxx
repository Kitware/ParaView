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
  auto delegate = this->GetFrameExporterDelegate();
  if (!delegate)
  {
    return false;
  }
  this->FrameCounter = this->StartFileCount;
  if (this->AnimationEnabled())
  {
    this->GetAnimationScene()->SetOverrideStillRender(true);
  }
  if (this->SingleFile)
  {
    // All frames share the same filename, and are combined into a single
    // file by the delegate exporter as it receives each frame.
    vtkSMPropertyHelper(delegate, "FileName").Set(this->GetFileName());
    delegate->UpdateVTKObjects();
    delegate->InvokeCommand("Start");
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneSeriesWriter::SaveFrame(double time)
{
  auto delegate = this->GetFrameExporterDelegate();
  if (this->SingleFile)
  {
    vtkSMPropertyHelper(delegate, "TimeValue").Set(time);
  }
  else
  {
    vtkSMPropertyHelper(delegate, "FileName").Set(this->BuildCurrentFilePath().c_str());
  }

  delegate->UpdateVTKObjects();
  delegate->GetView()->Update();
  delegate->Write();
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
  if (this->SingleFile)
  {
    this->GetFrameExporterDelegate()->InvokeCommand("Finish");
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
