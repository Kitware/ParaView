/*=========================================================================

   Program: ParaView
   Module:    pqImmediateExportReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqImmediateExportReaction.h"

#include "vtkCamera.h"
#include "vtkPVConfig.h"
#include "vtkSMExportProxyDepot.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include "pqApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"

#include <sstream>
#include <string>
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#include "vtkPythonInterpreter.h"
#endif

// clang-format off
namespace
{
static const char* export_now_code = R"DONTPARSE(
root_directory='%1'
file_name_padding=%2
make_cinema_table=%3
cinema_tracks={%4}
cinema_arrays={%5}
rendering_info={%6}

from paraview.detail import exportnow

exportnow.ExportNow(root_directory, file_name_padding, make_cinema_table, cinema_tracks, cinema_arrays, rendering_info)

)DONTPARSE";
}
// clang-format on

//-----------------------------------------------------------------------------
pqImmediateExportReaction::pqImmediateExportReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqImmediateExportReaction::~pqImmediateExportReaction()
{
}

//-----------------------------------------------------------------------------
void pqImmediateExportReaction::onTriggered()
{
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  vtkSMExportProxyDepot* ed = pxm->GetExportDepot();

  // global options
  vtkSMProxy* globaloptions = ed->GetGlobalOptions();
  QString root_directory = vtkSMPropertyHelper(globaloptions, "RootDirectory").GetAsString();
  int file_name_padding = vtkSMPropertyHelper(globaloptions, "FileNamePadding").GetAsInt();
  QString make_cinema_table =
    vtkSMPropertyHelper(globaloptions, "SaveDTable").GetAsInt(0) == 0 ? "False" : "True";

  // writers
  bool exported_any_writers = false;
  QString cinema_tracks = "";
  bool hasTrackSets = false;
  QString cinema_arrays = "";
  bool hasArraySets = false;
  ed->InitNextWriterProxy();
  while (auto nextWriter = ed->GetNextWriterProxy())
  {
    if (!nextWriter->HasAnnotation("enabled") ||
      strcmp(nextWriter->GetAnnotation("enabled"), "1") != 0)
    {
      continue;
    }

    if (strcmp(nextWriter->GetXMLName(), "Cinema image options") != 0)
    {
      exported_any_writers = true;
      continue;
    }

    // done here except cinema image specific parameters
    const char* hashname = pxm->GetProxyName("export_writers", nextWriter);
    std::string formatS = hashname;
    size_t underP = formatS.find_first_of("|");
    std::string filterName = formatS.substr(0, underP);
    filterName[0] = tolower(filterName[0]);

    int nelems;
    nelems = vtkSMPropertyHelper(nextWriter, "Cinema Parameters").GetNumberOfElements();
    if (nelems > 0)
    {
      hasTrackSets = true;
      QString thisTrack = QString("'");
      thisTrack += QString(filterName.c_str());
      thisTrack += QString("':[");
      std::stringstream track_cache;
      for (int i = 0; i < nelems; ++i)
      {
        track_cache << vtkSMPropertyHelper(nextWriter, "Cinema Parameters").GetAsDouble(i);
        track_cache << ", ";
      }
      thisTrack += QString(track_cache.str().c_str());
      thisTrack.chop(1);
      thisTrack += QString("],");
      cinema_tracks += thisTrack;
    }

    if (vtkSMPropertyHelper(nextWriter, "ChooseArraysToWrite").GetAsInt(0) == 1)
    {
      QString theseArrays = QString("'");
      theseArrays += QString(filterName.c_str());
      theseArrays += QString("':[");
      hasArraySets = true;
      // TODO: there isn't an API to distinguish cell and point arrays or the same name
      nelems = vtkSMPropertyHelper(nextWriter, "CellDataArrays").GetNumberOfElements();
      for (int i = 0; i < nelems; ++i)
      {
        theseArrays += "'";
        theseArrays += vtkSMPropertyHelper(nextWriter, "CellDataArrays").GetAsString(i);
        theseArrays += "',";
      }

      nelems = vtkSMPropertyHelper(nextWriter, "PointDataArrays").GetNumberOfElements();
      for (int i = 0; i < nelems; ++i)
      {
        theseArrays += "'";
        theseArrays += vtkSMPropertyHelper(nextWriter, "PointDataArrays").GetAsString(i);
        theseArrays += "',";
      }
      hasArraySets = true;
      theseArrays.chop(1);
      theseArrays += "],";
      cinema_arrays += theseArrays;
    }
  }
  if (hasTrackSets)
  {
    cinema_tracks.chop(1);
  }

  if (hasArraySets)
  {
    cinema_arrays.chop(1);
  }

  // screenshots
  bool exported_any_screenshots = false;
  QString rendering_info; // a map from the render view name to render output params
  ed->InitNextScreenshotProxy();
  while (auto nextScreenshot = ed->GetNextScreenshotProxy())
  {
    vtkSMSaveScreenshotProxy* ssProxy = vtkSMSaveScreenshotProxy::SafeDownCast(nextScreenshot);
    if (!ssProxy->HasAnnotation("enabled") || strcmp(ssProxy->GetAnnotation("enabled"), "1") != 0)
    {
      continue;
    }
    exported_any_screenshots = true;

    std::string filename = vtkSMPropertyHelper(ssProxy, "CatalystFilePattern").GetAsString();
    QString imagefilename = QString::fromStdString(filename);

    vtkSMProxy* viewProxy = ssProxy->GetView();
    QString viewname = pxm->GetProxyName("views", viewProxy);

    int wf = vtkSMPropertyHelper(ssProxy, "WriteFrequency").GetAsInt();
    QString frequency = QString::number(wf);

    // this is catalyst specific, need to add to ssProxy
    QString fitScreen =
      vtkSMPropertyHelper(ssProxy, "CatalystFitToScreen").GetAsInt(0) == 0 ? "0" : "1";

    // this was catalyst specific, I am dropping it from the UI for now
    QString magnification = "1";

    vtkVector2i targetSize;
    vtkSMPropertyHelper(ssProxy, "ImageResolution").Get(targetSize.GetData(), 2);
    QString width = QString::fromStdString(std::to_string(targetSize.GetX()));
    QString height = QString::fromStdString(std::to_string(targetSize.GetY()));

    // this is catalyst specific, need to add to SSSProxy
    // but will come from a new CinemaSpecific file format
    QString cinema_options = "{}";

    if (imagefilename.endsWith("cdb"))
    {
      // get the cinema database shape from the CDB subproxy
      vtkSMProxy* writerProxy = ssProxy->GetFormatProxy(filename);
      if (writerProxy)
      {
        imagefilename = imagefilename.replace("cdb", "png"); // just always use png for normal
                                                             // images
        int renderingLevel = vtkSMPropertyHelper(writerProxy, "DeferredRendering").GetAsInt();
        QString compositeState;
        QString useValues;
        QString valueFormat = "'floatValues':True"; // now that we can rely on float textures
                                                    // everywhere, never use the 0..2^24 hack
        switch (renderingLevel)
        {
          case 0:
          default:
            // spec A
            compositeState = "'composite':False";
            useValues = "'noValues':True";
            break;
          case 1:
            // spec C with RGB colors
            compositeState = "'composite':True";
            useValues = "'noValues':True";
            break;
          case 2:
            // spec C with float colors
            compositeState = "'composite':True";
            useValues = "'noValues':False";
            break;
        }
        int cameraLevel = vtkSMPropertyHelper(writerProxy, "CameraModel").GetAsInt();
        if (renderingLevel == 0 && cameraLevel > 1)
        {
          cameraLevel = 1;
          qWarning(
            "Camera Type choice is not compatible with Cinema Spec A, falling back to phi-theta.");
        }

        int numPhis = vtkSMPropertyHelper(writerProxy, "Camera Phi Divisions").GetAsInt();
        int numThetas = vtkSMPropertyHelper(writerProxy, "Camera Theta Divisions").GetAsInt();
        int numRolls = vtkSMPropertyHelper(writerProxy, "Camera Roll Divisions").GetAsInt();
        if (numRolls > 1)
        {
          if (renderingLevel == 0)
          {
            numRolls = 1;
            qWarning("Roll is not compatible with Cinema Spec A, falling back to roll = 1.");
          }
          else if (cameraLevel < 2)
          {
            numRolls = 1;
            qWarning("Roll requires pose based camera, falling back to roll = 1.");
          }
        }
        QString camState;
        QString phis = "";
        QString thetas = "";
        QString rolls = "";
        switch (cameraLevel)
        {
          case 0:
          default:
            camState = "'camera':'static'";
            break;
          case 1:
            camState = "'camera':'phi-theta'";
            if (numPhis < 2)
            {
              phis = "'phi':[0]";
            }
            else
            {
              phis = "'phi':[";
              for (int j = -180; j < 180; j += (360 / numPhis))
              {
                phis += QString::number(j) + ",";
              }
              phis.chop(1);
              phis += "]";
            }

            if (numThetas < 2)
            {
              thetas = "'theta':[0]";
            }
            else
            {
              thetas = "'theta':[";
              for (int j = -90; j < 90; j += (180 / numThetas))
              {
                thetas += QString::number(j) + ",";
              }
              thetas.chop(1);
              thetas += "]";
            }
            break;
          case 2:
            camState = "'camera':'azimuth-elevation-roll'";
            phis = "'phi':[" + QString::number(numPhis) + "]";
            thetas = "'theta':[" + QString::number(numThetas) + "]";
            rolls = "'roll':[" + QString::number(numRolls) + "]";
            break;
          case 3:
            camState = "'camera':'yaw-pitch-roll'";
            phis = "'phi':[" + QString::number(numPhis) + "]";
            thetas = "'theta':[" + QString::number(numThetas) + "]";
            rolls = "'roll':[" + QString::number(numRolls) + "]";
            break;
        }
        cinema_options =
          "{" + compositeState + "," + useValues + "," + valueFormat + "," + camState;
        if (phis != "")
        {
          cinema_options += "," + phis;
        }
        if (thetas != "")
        {
          cinema_options += "," + thetas;
        }
        if (rolls != "")
        {
          cinema_options += "," + rolls;
        }

        QString trackedObject = vtkSMPropertyHelper(writerProxy, "TrackObject").GetAsString();
        if (trackedObject != "None")
        {
          if (renderingLevel == 0)
          {
            trackedObject = "None";
            qWarning(
              "Object tracking is not compatible with Cinema Spec A, falling back to no tracking.");
          }
          else if (cameraLevel < 2)
          {
            trackedObject = "None";
            qWarning("Object tracking requires pose based camera, falling back to no tracking.");
          }
        }

        cinema_options += ", 'tracking':{ 'object':'" + trackedObject + "'}";

        vtkSMRenderViewProxy* rvp = vtkSMRenderViewProxy::SafeDownCast(viewProxy);
        vtkCamera* cam = rvp->GetActiveCamera();
        double eye[3];
        double at[3];
        double up[3];
        cam->GetPosition(eye);
        vtkSMPropertyHelper(rvp, "CenterOfRotation").Get(at, 3);
        cam->GetViewUp(up);
        cinema_options += ", 'initial':{ ";
        cinema_options += "'eye': [" + QString::number(eye[0]) + "," + QString::number(eye[1]) +
          "," + QString::number(eye[2]) + "], ";
        cinema_options += "'at': [" + QString::number(at[0]) + "," + QString::number(at[1]) + "," +
          QString::number(at[2]) + "], ";
        cinema_options += "'up': [" + QString::number(up[0]) + "," + QString::number(up[1]) + "," +
          QString::number(up[2]) + "] ";
        cinema_options += "}";

        cinema_options += "}";
      }
    }

    QString viewformat = "'%1' : ['%2', %3, %4, %5, %6, %7, %8]";
    QString nextview = viewformat.arg(viewname)
                         .arg(imagefilename)
                         .arg(frequency)
                         .arg(fitScreen)
                         .arg(magnification)
                         .arg(width)
                         .arg(height)
                         .arg(cinema_options);
    rendering_info += nextview + ",";
  }

  if (!(exported_any_screenshots || exported_any_writers))
  {
    qWarning("Nothing to export, use Export Inspector to configure what you want to write.");
  }
  else
  {
    QString command = export_now_code;
    command = command.arg(root_directory)
                .arg(file_name_padding)
                .arg(make_cinema_table)
                .arg(cinema_tracks)
                .arg(cinema_arrays)
                .arg(rendering_info);
// cerr << command.toStdString() << endl;

// ensure Python in initialized.
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
    vtkPythonInterpreter::Initialize();
    vtkPythonInterpreter::RunSimpleString(command.toLocal8Bit().data());
#else
    qWarning("Export Now requires PARAVIEW_ENABLE_PYTHON");
#endif
  }
}
