/*=========================================================================

   Program: ParaView
   Module:    pqCoreTestUtility.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pqCoreTestUtility.h"

#include <QApplication>
#include <QCommonStyle>
#include <QCoreApplication>
#include <QEvent>
#include <QFileInfo>
#include <QImage>
#include <QPixmap>
#include <QWidget>
#include <QtDebug>

#include "QtTestingConfigure.h"

#include "pqApplicationCore.h"
#include "pqCollaborationEventPlayer.h"
#include "pqColorButtonEventPlayer.h"
#include "pqColorButtonEventTranslator.h"
#include "pqColorDialogEventPlayer.h"
#include "pqColorDialogEventTranslator.h"
#include "pqConsoleWidgetEventPlayer.h"
#include "pqConsoleWidgetEventTranslator.h"
#include "pqEventDispatcher.h"
#include "pqFileDialogEventPlayer.h"
#include "pqFileDialogEventTranslator.h"
#include "pqFlatTreeViewEventPlayer.h"
#include "pqFlatTreeViewEventTranslator.h"
#include "pqImageUtil.h"
#include "pqLineEditEventPlayer.h"
#include "pqOptions.h"
#include "pqQVTKWidget.h"
#include "pqQVTKWidgetEventPlayer.h"
#include "pqQVTKWidgetEventTranslator.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "pqXMLEventObserver.h"
#include "pqXMLEventSource.h"
#include "vtkBMPWriter.h"
#include "vtkErrorCode.h"
#include "vtkImageDifference.h"
#include "vtkImageShiftScale.h"
#include "vtkJPEGWriter.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkPNMWriter.h"
#include "vtkPVConfig.h"
#include "vtkPVServerInformation.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkTIFFWriter.h"
#include "vtkTesting.h"
#include "vtkTrivialProducer.h"
#include "vtkWindowToImageFilter.h"
#include "vtksys/SystemTools.hxx"

#include <cassert>

#ifdef QT_TESTING_WITH_PYTHON
#include "pqPythonEventSourceImage.h"
#endif

const char* pqCoreTestUtility::PQ_COMPAREVIEW_PROPERTY_NAME = "PQ_COMPAREVIEW_PROPERTY_NAME";

template <typename WriterT>
bool saveImage(vtkWindowToImageFilter* Capture, const QFileInfo& File)
{
  WriterT* const writer = WriterT::New();
  writer->SetInputConnection(Capture->GetOutputPort());
  writer->SetFileName(File.filePath().toLocal8Bit().data());
  writer->Write();
  const bool result = writer->GetErrorCode() == vtkErrorCode::NoError;
  writer->Delete();

  return result;
}

/////////////////////////////////////////////////////////////////////////////
// pqCoreTestUtility

pqCoreTestUtility::pqCoreTestUtility(QObject* p)
  : pqTestUtility(p)
{
  // we don't want to the dispatcher to wait during event playback. We will
  // explicitly register timers that need to be timed out.
  pqEventDispatcher::setEventPlaybackDelay(0);

  // add an XML source
  this->addEventSource("xml", new pqXMLEventSource(this));
  this->addEventObserver("xml", new pqXMLEventObserver(this));

#ifdef QT_TESTING_WITH_PYTHON
  this->addEventSource("py", new pqPythonEventSourceImage(this));
#endif

  this->eventTranslator()->addWidgetEventTranslator(new pqQVTKWidgetEventTranslator(this));
  this->eventTranslator()->addWidgetEventTranslator(new pqFileDialogEventTranslator(this));
  this->eventTranslator()->addWidgetEventTranslator(new pqFlatTreeViewEventTranslator(this));
  this->eventTranslator()->addWidgetEventTranslator(new pqColorButtonEventTranslator(this));
  this->eventTranslator()->addWidgetEventTranslator(new pqColorDialogEventTranslator(this));
  this->eventTranslator()->addWidgetEventTranslator(new pqConsoleWidgetEventTranslator(this));

  this->eventPlayer()->addWidgetEventPlayer(new pqLineEditEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(new pqQVTKWidgetEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(new pqFileDialogEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(new pqFlatTreeViewEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(new pqColorButtonEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(new pqColorDialogEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(new pqCollaborationEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(new pqConsoleWidgetEventPlayer(this));
}

//-----------------------------------------------------------------------------
pqCoreTestUtility::~pqCoreTestUtility()
{
}

//-----------------------------------------------------------------------------
QString pqCoreTestUtility::DataRoot()
{
  QString result;
  if (pqOptions* const options =
        pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions()))
  {
    result = options->GetDataDirectory();
  }

  // Let the user override the defaults by setting an environment variable ...
  if (result.isEmpty())
  {
    result = vtksys::SystemTools::GetEnv("PARAVIEW_DATA_ROOT");
  }

  return result.isEmpty() ? result : QDir::cleanPath(result);
}

//-----------------------------------------------------------------------------
QString pqCoreTestUtility::BaselineDirectory()
{
  QString result;
  if (pqOptions* const options =
        pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions()))
  {
    result = options->GetBaselineDirectory();
  }

  // Let the user override the defaults by setting an environment variable ...
  if (result.isEmpty())
  {
    result = vtksys::SystemTools::GetEnv("PARAVIEW_TEST_BASELINE_DIR");
  }

  // Finally use the xml file location if an instance is available
  if (result.isEmpty())
  {
    pqApplicationCore* core = pqApplicationCore::instance();
    if (core != NULL)
    {
      pqTestUtility* testUtil = core->testUtility();
      result = QFileInfo(testUtil->filename()).path();
    }
  }

  // Use current repo in case non are provided
  if (result.isEmpty())
  {
    result = ".";
  }

  // Ensure all slashes face forward ...
  result.replace('\\', '/');

  // Remove any trailing slashes ...
  if (result.size() && result.at(result.size() - 1) == '/')
  {
    result.chop(1);
  }

  // Trim excess whitespace ...
  result = result.trimmed();

  return result;
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::SaveScreenshot(vtkRenderWindow* RenderWindow, const QString& File)
{
  vtkWindowToImageFilter* const capture = vtkWindowToImageFilter::New();
  capture->SetInput(RenderWindow);
  capture->Update();

  bool success = false;

  const QFileInfo file(File);
  if (file.completeSuffix() == "bmp")
    success = saveImage<vtkBMPWriter>(capture, file);
  else if (file.completeSuffix() == "tif")
    success = saveImage<vtkTIFFWriter>(capture, file);
  else if (file.completeSuffix() == "ppm")
    success = saveImage<vtkPNMWriter>(capture, file);
  else if (file.completeSuffix() == "png")
    success = saveImage<vtkPNGWriter>(capture, file);
  else if (file.completeSuffix() == "jpg")
    success = saveImage<vtkJPEGWriter>(capture, file);

  capture->Delete();

  return success;
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareImage(vtkRenderWindow* RenderWindow, const QString& ReferenceImage,
  double Threshold, ostream& vtkNotUsed(Output), const QString& TempDirectory)
{
  vtkSmartPointer<vtkTesting> testing = vtkSmartPointer<vtkTesting>::New();
  testing->AddArgument("-T");
  testing->AddArgument(TempDirectory.toLocal8Bit().data());
  testing->AddArgument("-V");
  testing->AddArgument(ReferenceImage.toLocal8Bit().data());
  testing->SetRenderWindow(RenderWindow);
  if (testing->RegressionTest(Threshold) == vtkTesting::PASSED)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareImage(vtkImageData* testImage, const QString& ReferenceImage,
  double Threshold, ostream& vtkNotUsed(Output), const QString& TempDirectory)
{
  vtkSmartPointer<vtkTesting> testing = vtkSmartPointer<vtkTesting>::New();
  testing->AddArgument("-T");
  testing->AddArgument(TempDirectory.toLocal8Bit().data());
  testing->AddArgument("-V");
  testing->AddArgument(ReferenceImage.toLocal8Bit().data());
  vtkSmartPointer<vtkTrivialProducer> tp = vtkSmartPointer<vtkTrivialProducer>::New();
  tp->SetOutput(testImage);
  if (testing->RegressionTest(tp, Threshold) == vtkTesting::PASSED)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareImage(QWidget* widget, const QString& referenceImage,
  double threshold, ostream& vtkNotUsed(output), const QString& tempDirectory,
  const QSize& size /*=QSize(300, 300)*/)
{
  assert(widget != NULL);

  // try to locate a pqView, if any associated with the QWidget.
  QList<pqView*> views =
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqView*>();
  foreach (pqView* view, views)
  {
    if (view && (view->widget() == widget))
    {
      cout << "Using View API for capture" << endl;
      return pqCoreTestUtility::CompareView(view, referenceImage, threshold, tempDirectory, size);
    }
  }

  qFatal("CompareImage not supported!");
  return false;
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareView(pqView* curView, const QString& referenceImage,
  double threshold, const QString& tempDirectory, const QSize& size /*=QSize()*/)
{
  assert(curView != NULL);

  auto viewProxy = curView->getViewProxy();

  // update size and 2d text dpi for tests.
  int original_size[2];
  vtkSMPropertyHelper sizeHelper(viewProxy, "ViewSize");
  sizeHelper.Get(original_size, 2);

  vtkSMPropertyHelper dpiHelper(viewProxy, "PPI");
  const int original_dpi = dpiHelper.GetAsInt();

  if (size.isValid() && !size.isEmpty())
  {
    const int new_size[2] = { size.width(), size.height() };
    sizeHelper.Set(new_size, 2);
    dpiHelper.Set(72); // fixed DPI for testing.
  }
  viewProxy->UpdateVTKObjects();

  auto test_image = vtkSmartPointer<vtkImageData>::Take(viewProxy->CaptureWindow(1));

  // restore size and dpi.
  sizeHelper.Set(original_size, 2);
  dpiHelper.Set(original_dpi);
  viewProxy->UpdateVTKObjects();

  curView->widget()->update();

  if (!test_image)
  {
    qCritical() << "ERROR: Failed to capture snapshot.";
    return false;
  }

  // The returned image will have extents translated to match the view position,
  // we shift them back.
  int view_position[2];
  vtkSMPropertyHelper(viewProxy, "ViewPosition").Get(view_position, 2);
  // Update image extents based on ViewPosition
  int extents[6];
  test_image->GetExtent(extents);
  for (int cc = 0; cc < 4; cc++)
  {
    extents[cc] -= view_position[cc / 2];
  }
  test_image->SetExtent(extents);
  bool ret =
    pqCoreTestUtility::CompareImage(test_image, referenceImage, threshold, cout, tempDirectory);
  return ret;
}

//-----------------------------------------------------------------------------
QString pqCoreTestUtility::TestDirectory()
{
  QString result;
  if (pqOptions* const options =
        pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions()))
  {
    result = options->GetTestDirectory();
  }

  // Let the user override the defaults by setting an environment variable ...
  if (result.isEmpty())
  {
    result = vtksys::SystemTools::GetEnv("PARAVIEW_TEST_DIR");
  }

  // Ensure all slashes face forward ...
  result.replace('\\', '/');

  // Remove any trailing slashes ...
  if (result.size() && result.at(result.size() - 1) == '/')
  {
    result.chop(1);
  }

  // Trim excess whitespace ...
  result = result.trimmed();

  return result;
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareImage(const QString& testPNGImage, const QString& referenceImage,
  double threshold, ostream& output, const QString& tempDirectory)
{
  vtkNew<vtkPNGReader> reader;
  if (!reader->CanReadFile(testPNGImage.toLocal8Bit().data()))
  {
    output << "Cannot read file : " << testPNGImage.toLocal8Bit().data() << endl;
    return false;
  }
  reader->SetFileName(testPNGImage.toLocal8Bit().data());
  reader->Update();
  return pqCoreTestUtility::CompareImage(
    reader->GetOutput(), referenceImage, threshold, output, tempDirectory);
}

//-----------------------------------------------------------------------------
QString pqCoreTestUtility::fixPath(const QString& path)
{
  QString newpath = path;
  newpath.replace("$PARAVIEW_TEST_ROOT", pqCoreTestUtility::TestDirectory());
  newpath.replace("$PARAVIEW_TEST_BASELINE_DIR", pqCoreTestUtility::BaselineDirectory());
  newpath.replace("$PARAVIEW_DATA_ROOT", pqCoreTestUtility::DataRoot());
  return newpath;
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareTile(QWidget* widget, int rank, int tdx, int tdy,
  const QString& baseline, double threshold, ostream& output, const QString& tempDirectory)
{
  // try to locate a pqView, if any associated with the QWidget.
  auto views = pqApplicationCore::instance()->getServerManagerModel()->findItems<pqView*>();
  for (pqView* view : views)
  {
    if (view && (view->widget() == widget))
    {
      return pqCoreTestUtility::CompareTile(
        view, rank, tdx, tdy, baseline, threshold, output, tempDirectory);
    }
  }

  qFatal("CompareTile not supported on the provided widget");
  return false;
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareTile(pqView* view, int rank, int tdx, int tdy,
  const QString& baseline, double threshold, ostream& output, const QString& tempDirectory)
{
  auto layout = view ? vtkSMViewLayoutProxy::FindLayout(view->getViewProxy()) : nullptr;
  if (!layout)
  {
    return false;
  }

  if (tdx >= 1 && tdy >= 1)
  {
    auto serverInfo = view->getServer()->getServerInformation();
    if (!serverInfo || serverInfo->GetTileDimensions()[0] != tdx ||
      serverInfo->GetTileDimensions()[1] != tdy)
    {
      // skip compare.
      return true;
    }
  }

  const QString imagepath =
    QString("%1/tile-%2.png").arg(tempDirectory).arg(QFileInfo(baseline).baseName());
  layout->SaveAsPNG(rank, imagepath.toLocal8Bit().data());
  return pqCoreTestUtility::CompareImage(imagepath, baseline, threshold, output, tempDirectory);
}
