/*=========================================================================

   Program: ParaView
   Module:    pqPythonEventSourceImage.cxx

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

// python header first
#include "vtkPython.h"
#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC extern "C" void
#endif // PyMODINIT_FUNC

// self include
#include "pqPythonEventSourceImage.h"

// Qt include
#include <QPixmap>
#include <QImage>
#include <QWidget>
#include <QCoreApplication>
#include <QEvent>
#include <QDir>
#include <QCommonStyle>

// Qt testing includes
#include "pqObjectNaming.h"
#include "pqTestUtility.h"
#include "pqEventDispatcher.h"

// VTK includes
#include "vtkImageData.h"
#include "vtkPNGReader.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"

// SM includes
#include "vtkProcessModule.h"

// pqCore includes
#include "pqCoreTestUtility.h"
#include "pqOptions.h"
#include "pqImageUtil.h"


// since we have only one instance at a time
static pqPythonEventSourceImage* Instance = 0;
bool SnapshotResult = false;
QString SnapshotWidget;
QString SnapshotBaseline;
QString SnapshotTestImage;
int SnapshotWidth = 1;
int SnapshotHeight = 1;

static PyObject*
QtTestingImage_compareImage(PyObject* /*self*/, PyObject* args)
{
  // void QtTestingImage.compareImage('png file', 'baselineFile')   or
  // void QtTestingImage.compareImage('object', 'baselineFile', width, height)
  //   an exception is thrown in this fails
 
  pqThreadedEventSource::msleep(1000);

  const char* object = 0;
  const char* baseline = 0;
  const char* pngfile = 0;
  int width = 0;
  int height = 0;
  bool image_image_compare = false;

  if(!PyArg_ParseTuple(args, 
                       const_cast<char*>("ssii"), 
                       &object, &baseline, &width, &height))
    {
    if(PyArg_ParseTuple(args, 
        const_cast<char*>("ss"), &pngfile, &baseline))
      {
      image_image_compare = true;
      }
    else
      {
      PyErr_SetString(PyExc_TypeError, "bad arguments to compareImage()");
      return NULL;
      }
    }

  SnapshotResult = false;
  SnapshotWidget = object;
  SnapshotBaseline = baseline;
  SnapshotWidth = width;
  SnapshotHeight = height;
  SnapshotTestImage = pngfile;

  // get our routines on the GUI thread to do the image comparison
  QMetaObject::invokeMethod(Instance, "doComparison", Qt::QueuedConnection);

  // wait for image comparison results
  if(!Instance->waitForGUI())
    {
    PyErr_SetString(PyExc_ValueError, "error during image comparison");
    return NULL;
    }

  if(!image_image_compare && SnapshotWidget == QString::null)
    {
    PyErr_SetString(PyExc_ValueError, "object not found");
    return NULL;
    }

  if(!SnapshotResult)
    {
    PyErr_SetString(PyExc_ValueError, "image comparison failed");
    return NULL;
    }

  return Py_BuildValue(const_cast<char*>(""));
}

static PyMethodDef QtTestingImageMethods[] = {
  {
    const_cast<char*>("compareImage"), 
    QtTestingImage_compareImage,
    METH_VARARGS,
    const_cast<char*>("compare the snapshot of a widget/image with a baseline")
  },

  {NULL, NULL, 0, NULL} // Sentinal
};

PyMODINIT_FUNC
initQtTestingImage(void)
{
  Py_InitModule(const_cast<char*>("QtTestingImage"), QtTestingImageMethods);
}


pqPythonEventSourceImage::pqPythonEventSourceImage(QObject* p)
  : pqPythonEventSource(p)
{
  // add QtTesting to python's inittab, so it is
  // available to all interpreters
  PyImport_AppendInittab(const_cast<char*>("QtTestingImage"),
                         initQtTestingImage);
}

pqPythonEventSourceImage::~pqPythonEventSourceImage()
{
}

void pqPythonEventSourceImage::run()
{
  Instance = this;
  pqPythonEventSource::run();
}

//-----------------------------------------------------------------------------
void pqPythonEventSourceImage::doComparison()
{
  // make sure all other processing has been done before we take a snapshot
  pqEventDispatcher::processEventsAndWait(10);

  // assume all images are in the dataroot/Baseline directory
  QString fullpath = pqCoreTestUtility::DataRoot();
  fullpath += "/Baseline/";
  fullpath += SnapshotBaseline;

  pqOptions* const options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  int threshold = options->GetCurrentImageThreshold();
  QString testdir = options->GetTestDirectory();
  if(testdir == QString::null)
    {
    testdir = ".";
    }

  if (SnapshotWidget != QString::null)
    {
    QWidget* widget =
      qobject_cast<QWidget*>(pqObjectNaming::GetObject(SnapshotWidget));
    if(widget)
      {
      this->compareImage(widget,
                          fullpath,
                          threshold,
                          testdir);
      }
    }
  else if (SnapshotTestImage != QString::null)
    {
    SnapshotTestImage = SnapshotTestImage.replace("$PARAVIEW_TEST_ROOT",
      pqCoreTestUtility::TestDirectory());
    SnapshotTestImage = SnapshotTestImage.replace("$PARAVIEW_DATA_ROOT",
      pqCoreTestUtility::DataRoot());
    this->compareImage(SnapshotTestImage, fullpath, threshold, testdir);
    }

  // signal the testing thread
  this->guiAcknowledge();
}

//-----------------------------------------------------------------------------
void pqPythonEventSourceImage::compareImage(QWidget* widget,
                    const QString& baseline,
                    double threshold,
                    const QString& tempDir)
{
  
  // for generic QWidget's, let's paint the widget into our QPixmap,
  // put it in a vtkImageData and compare the image with a baseline

  // grab an image of the widget
  QSize oldSize = widget->size();
  widget->resize(SnapshotWidth, SnapshotHeight);
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
  widget->resize(oldSize);
  widget->setFont(oldFont);
  widget->setStyle(oldStyle);
 
  vtkSmartPointer<vtkImageData> vtkimage = vtkSmartPointer<vtkImageData>::New();
  pqImageUtil::toImageData(img, vtkimage);

  this->compareImageInternal(vtkimage, baseline, threshold, tempDir);
}

//-----------------------------------------------------------------------------
void pqPythonEventSourceImage::compareImageInternal(vtkImageData* vtkimage,
  const QString& baseline, double threshold, const QString& tempDir)
{
  vtkSmartPointer<vtkTesting> testing = vtkSmartPointer<vtkTesting>::New();
  testing->AddArgument("-T");
  testing->AddArgument(tempDir.toAscii().data());
  testing->AddArgument("-V");
  testing->AddArgument(baseline.toAscii().data());

  // compare the image
  SnapshotResult = 
    (testing->RegressionTest(vtkimage, threshold) == vtkTesting::PASSED);
}

//-----------------------------------------------------------------------------
void pqPythonEventSourceImage::compareImage(const QString& image,
  const QString& baseline, double threshold, const QString& tempDir)
{
  vtkSmartPointer<vtkPNGReader> reader = vtkSmartPointer<vtkPNGReader>::New();
  if (!reader->CanReadFile(image.toAscii().data()))
    {
    qCritical("cannot read file %s\n", image.toAscii().data());
    SnapshotResult = false;
    return;
    }
  reader->SetFileName(image.toAscii().data());
  reader->Update();
  this->compareImageInternal(reader->GetOutput(), baseline, threshold, tempDir);
}

