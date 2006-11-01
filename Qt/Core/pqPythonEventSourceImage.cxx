/*=========================================================================

   Program: ParaView
   Module:    pqPythonEventSourceImage.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

// self include
#include "pqPythonEventSourceImage.h"

// Qt include
#include <QPixmap>
#include <QImage>
#include <QWidget>
#include <QWaitCondition>
#include <QMutex>
#include <QCoreApplication>
#include <QEvent>
#include <QDir>

// Qt testing includes
#include "pqObjectNaming.h"
#include "pqTestUtility.h"

// VTK includes
#include "vtkSmartPointer.h"
#include "vtkTesting.h"
#include "vtkImageData.h"

// SM includes
#include "vtkProcessModule.h"

// pqCore includes
#include "pqCoreTestUtility.h"
#include "pqOptions.h"


// since we have only one instance at a time
static pqPythonEventSourceImage* Instance = 0;
static QWaitCondition WaitForSnapshot;
bool SnapshotResult = false;
QString SnapshotWidget;
QString SnapshotBaseline;
int SnapshotWidth = 1;
int SnapshotHeight = 1;

static PyObject*
QtTestingImage_compareImage(PyObject* /*self*/, PyObject* args)
{
  // void QtTestingImage.compareImage('object', 'baselineFile', width, height)
  //   an exception is thrown in this fails
  
  const char* object = 0;
  const char* baseline = 0;
  int width = 0;
  int height = 0;

  if(!PyArg_ParseTuple(args, 
                       const_cast<char*>("ssii"), 
                       &object, &baseline, &width, &height))
    {
    PyErr_SetString(PyExc_TypeError, "bad arguments to compareImage()");
    return NULL;
    }

  SnapshotResult = false;
  SnapshotWidget = object;
  SnapshotBaseline = baseline;
  SnapshotWidth = width;
  SnapshotHeight = height;

  QMutex mut;
  mut.lock();

  // get our routines on the GUI thread to do the image comparison
  QMetaObject::invokeMethod(Instance, "doSnapshot", Qt::QueuedConnection);

  // wait for image comparison results
  WaitForSnapshot.wait(&mut);

  if(SnapshotWidget == QString::null)
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
    const_cast<char*>("compare the snapshot of a widget with a baseline")
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
  // initialize the QtTestingImage module
  initQtTestingImage();
}

pqPythonEventSourceImage::~pqPythonEventSourceImage()
{
}

void pqPythonEventSourceImage::run()
{
  Instance = this;
  pqPythonEventSource::run();
}

void pqPythonEventSourceImage::doSnapshot()
{
  QWidget* widget =
    qobject_cast<QWidget*>(pqObjectNaming::GetObject(SnapshotWidget));
  if(!widget)
    {
    SnapshotWidget = QString::null;
    }
  else
    {
    // assume all images are in the dataroot/Baseline directory
    QString fullpath = pqCoreTestUtility::DataRoot();
    fullpath += "/Baseline/";
    fullpath += SnapshotBaseline;


    pqOptions* const options = pqOptions::SafeDownCast(
          vtkProcessModule::GetProcessModule()->GetOptions());
    int threshold = options->GetImageThreshold();
    QString testdir = options->GetTestDirectory();
    if(testdir == QString::null)
      {
      testdir = ".";
      }

    this->compareImage(widget,
                        fullpath,
                        threshold,
                        testdir);
    }
  // signal the testing thread
  WaitForSnapshot.wakeAll();
}

void pqPythonEventSourceImage::compareImage(QWidget* widget,
                    const QString& baseline,
                    double threshold,
                    const QString& tempDir)
{
  
  // for generic QWidget's, let's paint the widget into our QPixmap,
  // put it in a vtkImageData and compare the image with a baseline
  vtkSmartPointer<vtkTesting> testing = vtkSmartPointer<vtkTesting>::New();
  testing->AddArgument("-T");
  testing->AddArgument(tempDir.toAscii().data());
  testing->AddArgument("-V");
  testing->AddArgument(baseline.toAscii().data());

  // grab an image of the widget
  QSize oldSize = widget->size();
  widget->resize(SnapshotWidth, SnapshotHeight);
  QFont oldFont = widget->font();
#if defined(Q_WS_WIN)
  QFont newFont("Arial", 8, QFont::Normal, false);
#elif defined(Q_WS_X11)
  QFont newFont("Courier", 7, QFont::Normal, false);
#else
  QFont newFont("Courier Regular", 10, QFont::Normal, false);
#endif
  widget->setFont(newFont);
  widget->repaint();
  QImage img = QPixmap::grabWidget(widget).toImage();
  widget->resize(oldSize);
  widget->setFont(oldFont);
  img = img.convertToFormat(QImage::Format_RGB32);
  img = img.mirrored();
  int width = img.width();
  int height = img.height();

  // copy the QImage to a vtkImageData
  vtkSmartPointer<vtkImageData> vtkimage = vtkSmartPointer<vtkImageData>::New();
  vtkimage->SetWholeExtent(0, width-1, 0, height-1, 0, 0); 
  vtkimage->SetSpacing(1.0, 1.0, 1.0);
  vtkimage->SetOrigin(0.0, 0.0, 0.0);
  vtkimage->SetNumberOfScalarComponents(3);
  vtkimage->SetScalarType(VTK_UNSIGNED_CHAR);
  vtkimage->SetExtent(vtkimage->GetWholeExtent());
  vtkimage->AllocateScalars();
  for(int i=0; i<height; i++)
    {
    unsigned char* ptr = static_cast<unsigned
      char*>(vtkimage->GetScalarPointer(0, i, 0));
    QRgb* linePixels = reinterpret_cast<QRgb*>(img.scanLine(i));
    for(int j=0; j<width; j++)
      {
      QRgb& col = linePixels[j];
      ptr[j*3] = qRed(col);
      ptr[j*3+1] = qGreen(col);
      ptr[j*3+2] = qBlue(col);
      }
    }

  // compare the image
  SnapshotResult = testing->RegressionTest(vtkimage, threshold) == vtkTesting::PASSED;

}


