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
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QImage>
#include <QPixmap>
#include <QtDebug>
#include <QWidget>

#include "QtTestingConfigure.h"

#include "pqApplicationCore.h"
#include "pqColorButtonEventPlayer.h"
#include "pqColorButtonEventTranslator.h"
#include "pqFileDialogEventPlayer.h"
#include "pqFileDialogEventTranslator.h"
#include "pqFlatTreeViewEventPlayer.h"
#include "pqFlatTreeViewEventTranslator.h"
#include "pqImageUtil.h"
#include "pqOptions.h"
#include "pqQVTKWidgetEventPlayer.h"
#include "pqQVTKWidgetEventTranslator.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "pqXMLEventObserver.h"
#include "pqXMLEventSource.h"
#include "vtkBMPWriter.h"
#include "vtkErrorCode.h"
#include "vtkImageDifference.h"
#include "vtkImageShiftScale.h"
#include "vtkJPEGWriter.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkPNMWriter.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMViewProxy.h"
#include "vtkTesting.h"
#include "vtkTIFFWriter.h"
#include "vtkWindowToImageFilter.h"

#ifdef QT_TESTING_WITH_PYTHON
# include "pqPythonEventSourceImage.h"
#endif


template<typename WriterT>
bool saveImage(vtkWindowToImageFilter* Capture, const QFileInfo& File)
{
  WriterT* const writer = WriterT::New();
  writer->SetInput(Capture->GetOutput());
  writer->SetFileName(File.filePath().toAscii().data());
  writer->Write();
  const bool result = writer->GetErrorCode() == vtkErrorCode::NoError;
  writer->Delete();
  
  return result;
}

/////////////////////////////////////////////////////////////////////////////
// pqCoreTestUtility

pqCoreTestUtility::pqCoreTestUtility(QObject* p) :
  pqTestUtility(p)
{
  // add an XML source
  this->addEventSource("xml", new pqXMLEventSource(this));
  this->addEventObserver("xml", new pqXMLEventObserver(this));

#ifdef QT_TESTING_WITH_PYTHON
  this->addEventSource("py", new pqPythonEventSourceImage(this));
#endif

  this->eventTranslator()->addWidgetEventTranslator(
       new pqQVTKWidgetEventTranslator(this));
  this->eventTranslator()->addWidgetEventTranslator(
       new pqFileDialogEventTranslator(this));
  this->eventTranslator()->addWidgetEventTranslator(
       new pqFlatTreeViewEventTranslator(this));
  this->eventTranslator()->addWidgetEventTranslator(
       new pqColorButtonEventTranslator(this));

  this->eventPlayer()->addWidgetEventPlayer(
       new pqQVTKWidgetEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(
       new pqFileDialogEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(
       new pqFlatTreeViewEventPlayer(this));
  this->eventPlayer()->addWidgetEventPlayer(
       new pqColorButtonEventPlayer(this));
}

//-----------------------------------------------------------------------------
pqCoreTestUtility::~pqCoreTestUtility()
{
}

//-----------------------------------------------------------------------------
QString pqCoreTestUtility::DataRoot()
{
  QString result;
  if (pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions()))
    {
    result = options->GetDataDirectory();
    }

  // Let the user override the defaults by setting an environment variable ...
  if(result.isEmpty())
    {
    result = getenv("PARAVIEW_DATA_ROOT");
    }
  
  // Otherwise, go with the compiled-in default ...
  if(result.isEmpty())
    {
    result = PARAVIEW_DATA_ROOT;
    }
  
  // Ensure all slashes face forward ...
  result.replace('\\', '/');
  
  // Remove any trailing slashes ...
  if(result.size() && result.at(result.size()-1) == '/')
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
  if(file.completeSuffix() == "bmp")
    success = saveImage<vtkBMPWriter>(capture, file);
  else if(file.completeSuffix() == "tif")
    success = saveImage<vtkTIFFWriter>(capture, file);
  else if(file.completeSuffix() == "ppm")
    success = saveImage<vtkPNMWriter>(capture, file);
  else if(file.completeSuffix() == "png")
    success = saveImage<vtkPNGWriter>(capture, file);
  else if(file.completeSuffix() == "jpg")
    success = saveImage<vtkJPEGWriter>(capture, file);
    
  capture->Delete();
  
  return success;
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareImage(vtkRenderWindow* RenderWindow, 
  const QString& ReferenceImage, double Threshold, 
  ostream& vtkNotUsed(Output), const QString& TempDirectory)
{
  vtkSmartPointer<vtkTesting> testing = vtkSmartPointer<vtkTesting>::New();
  testing->AddArgument("-T");
  testing->AddArgument(TempDirectory.toAscii().data());
  testing->AddArgument("-V");
  testing->AddArgument(ReferenceImage.toAscii().data());
  testing->SetRenderWindow(RenderWindow);
  if (testing->RegressionTest(Threshold) == vtkTesting::PASSED)
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareImage(vtkImageData* testImage,
  const QString& ReferenceImage, double Threshold, 
  ostream& vtkNotUsed(Output), const QString& TempDirectory)
{
  vtkSmartPointer<vtkTesting> testing = vtkSmartPointer<vtkTesting>::New();
  testing->AddArgument("-T");
  testing->AddArgument(TempDirectory.toAscii().data());
  testing->AddArgument("-V");
  testing->AddArgument(ReferenceImage.toAscii().data());
  if (testing->RegressionTest(testImage, Threshold) == vtkTesting::PASSED)
    {
    return true;
    }
  return false;
}

namespace pqCoreTestUtilityInternal{
  class WidgetSizer
    {
    QSize OldSize;
    QWidget* Widget;
  public:
    WidgetSizer(QWidget* widget, const QSize& size)
      {
      this->OldSize = widget->size();
      this->Widget = widget;
      widget->resize(size);
      }
    ~WidgetSizer()
      {
      this->Widget->resize(this->OldSize); 
      }

    };
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareImage(QWidget* widget,
  const QString& referenceImage, double threshold, 
  ostream& output, const QString& tempDirectory,
  const QSize& size /*=QSize(300, 300)*/)
{
  Q_ASSERT(widget != NULL);
  pqCoreTestUtilityInternal::WidgetSizer sizer(widget, size);

  // try to locate a pqView, if any associated with the QWidget.
  QList<pqView*> views =
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqView*>();
  foreach (pqView* view, views)
    {
    if (view && (view->getWidget() == widget))
      {
      cout << "Using View API for capture" << endl;
      return pqCoreTestUtility::CompareView(view, referenceImage, threshold,
        tempDirectory);
      }
    }

  // for generic QWidget's, let's paint the widget into our QPixmap,
  // put it in a vtkImageData and compare the image with a baseline
  QFont oldFont = widget->font();
#if defined(Q_WS_WIN)
  QFont newFont("Courier", 10, QFont::Normal, false);
#elif defined(Q_WS_X11)
  QFont newFont("Courier", 10, QFont::Normal, false);
#else
  QFont newFont("Courier Regular", 10, QFont::Normal, false);
#endif
  QCommonStyle style;
  QStyle* oldStyle = widget->style();
  widget->setStyle(&style);
  widget->setFont(newFont);
  QImage img = QPixmap::grabWidget(widget).toImage();
  widget->setFont(oldFont);
  widget->setStyle(oldStyle);

  vtkSmartPointer<vtkImageData> vtkimage = vtkSmartPointer<vtkImageData>::New();
  pqImageUtil::toImageData(img, vtkimage);

  return pqCoreTestUtility::CompareImage(vtkimage,
    referenceImage, threshold, output, tempDirectory);
}

//-----------------------------------------------------------------------------
bool pqCoreTestUtility::CompareView(
  pqView* curView,
  const QString& referenceImage,
  double threshold,
  const QString& tempDirectory)
{
  Q_ASSERT(curView != NULL);

  vtkImageData* test_image = curView->captureImage(1);
  if (!test_image)
    {
    qCritical() << "ERROR: Failed to capture snapshot.";
    return false;
    }

  // The returned image will have extents translated to match the view position,
  // we shift them back.
  int viewPos[2];
  vtkSMPropertyHelper(curView->getViewProxy(), "ViewPosition").Get(viewPos, 2);
  // Update image extents based on ViewPosition
  int extents[6];
  test_image->GetExtent(extents);
  for (int cc=0; cc < 4; cc++)
    {
    extents[cc] -= viewPos[cc/2];
    }
  test_image->SetExtent(extents);
  bool ret = pqCoreTestUtility::CompareImage(test_image, referenceImage,
    threshold, cout, tempDirectory);
  test_image->Delete();
  return ret;
}

//-----------------------------------------------------------------------------
QString pqCoreTestUtility::TestDirectory()
{
  if (pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions()))
    {
    return options->GetTestDirectory();
    }
  return QString();
}


