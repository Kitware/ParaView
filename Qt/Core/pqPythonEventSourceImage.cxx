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
#include <QCoreApplication>
#include <QEvent>
#include <QWidget>

// Qt testing includes
#include "pqEventDispatcher.h"
#include "pqObjectNaming.h"
#include "pqTestUtility.h"

// SM includes
#include "vtkProcessModule.h"

// pqCore includes
#include "pqCoreTestUtility.h"
#include "pqOptions.h"

// since we have only one instance at a time
static pqPythonEventSourceImage* Instance = 0;
bool SnapshotResult = false;
QString SnapshotWidget;
QString SnapshotBaseline;
QString SnapshotTestImage;
int SnapshotWidth = 1;
int SnapshotHeight = 1;

//-----------------------------------------------------------------------------
static PyObject* QtTestingImage_compareImage(PyObject* /*self*/, PyObject* args)
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

  if (!PyArg_ParseTuple(args, const_cast<char*>("ssii"), &object, &baseline, &width, &height))
  {
    if (PyArg_ParseTuple(args, const_cast<char*>("ss"), &pngfile, &baseline))
    {
      image_image_compare = true;
    }
    else
    {
      PyErr_SetString(PyExc_TypeError, "bad arguments to compareImage()");
      return nullptr;
    }
  }

  SnapshotResult = false;
  SnapshotWidget = object;
  SnapshotBaseline = baseline;
  SnapshotWidth = width;
  SnapshotHeight = height;
  SnapshotTestImage = pngfile;

  // get our routines on the GUI thread to do the image comparison
  QMetaObject::invokeMethod(Instance, "doComparison", Qt::BlockingQueuedConnection);

  // I'm going to start using Qt::BlockingQueuedConnection instead of
  // pqThreadedEventSource::waitForGUI(). We should deprecate that.
  // Qt::BlockingQueuedConnection will "block until the slot returns".

  if (!image_image_compare && SnapshotWidget.isNull())
  {
    PyErr_SetString(PyExc_ValueError, "object not found");
    return nullptr;
  }

  if (!SnapshotResult)
  {
    PyErr_SetString(PyExc_ValueError, "image comparison failed");
    return nullptr;
  }

  return Py_BuildValue(const_cast<char*>(""));
}

//-----------------------------------------------------------------------------
static PyMethodDef QtTestingImageMethods[] = {
  { const_cast<char*>("compareImage"), QtTestingImage_compareImage, METH_VARARGS,
    const_cast<char*>("compare the snapshot of a widget/image with a baseline") },

  { nullptr, nullptr, 0, nullptr } // Sentinel
};

//-----------------------------------------------------------------------------
PyMODINIT_FUNC initQtTestingImage(void)
{
  Py_InitModule(const_cast<char*>("QtTestingImage"), QtTestingImageMethods);
}

//-----------------------------------------------------------------------------
pqPythonEventSourceImage::pqPythonEventSourceImage(QObject* p)
  : pqPythonEventSource(p)
{
  // add QtTesting to python's inittab, so it is
  // available to all interpreters
  PyImport_AppendInittab(const_cast<char*>("QtTestingImage"), initQtTestingImage);
}

//-----------------------------------------------------------------------------
pqPythonEventSourceImage::~pqPythonEventSourceImage() = default;

//-----------------------------------------------------------------------------
void pqPythonEventSourceImage::run()
{
  Instance = this;
  pqPythonEventSource::run();
}

//-----------------------------------------------------------------------------
void pqPythonEventSourceImage::doComparison()
{
  // make sure all other processing has been done before we take a snapshot
  pqEventDispatcher::processEventsAndWait(500);

  // assume all images are in the dataroot/Baseline directory
  QString baseline_image = pqCoreTestUtility::DataRoot();
  baseline_image += "/Baseline/";
  baseline_image += SnapshotBaseline;

  pqOptions* const options =
    pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions());
  int threshold = options->GetCurrentImageThreshold();

  QString test_directory = pqCoreTestUtility::TestDirectory();
  if (test_directory.isNull())
  {
    test_directory = ".";
  }

  if (!SnapshotWidget.isNull())
  {
    QWidget* widget = qobject_cast<QWidget*>(pqObjectNaming::GetObject(SnapshotWidget));
    if (widget)
    {
      widget->resize(SnapshotWidth, SnapshotHeight);
      ::SnapshotResult = pqCoreTestUtility::CompareImage(widget, baseline_image, threshold,
        std::cerr, test_directory, QSize(SnapshotWidth, SnapshotHeight));
    }
  }
  else if (!SnapshotTestImage.isNull())
  {
    SnapshotTestImage = SnapshotTestImage.replace("$PARAVIEW_TEST_ROOT", test_directory);
    SnapshotTestImage =
      SnapshotTestImage.replace("$PARAVIEW_DATA_ROOT", pqCoreTestUtility::DataRoot());
    ::SnapshotResult = pqCoreTestUtility::CompareImage(
      SnapshotTestImage, baseline_image, threshold, std::cerr, test_directory);
  }
}
