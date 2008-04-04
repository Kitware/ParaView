/*=========================================================================

   Program: ParaView
   Module:    pqPythonEventSourceImage.h

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

#ifndef _pqPythonEventSourceImage_h
#define _pqPythonEventSourceImage_h

#include "pqPythonEventSource.h"
#include "pqCoreExport.h"

/** Python event source with Image comparison capabilities 

 import QtTestingImage

 QtTestingImage.compareView('widgetName', 'baseline', threshold, 'tempDir')
 
 */
class vtkImageData;

class PQCORE_EXPORT pqPythonEventSourceImage :
  public pqPythonEventSource
{
  Q_OBJECT
public:
  pqPythonEventSourceImage(QObject* p=0);
  ~pqPythonEventSourceImage();

public slots:
  // Compare the view in the \c widget with the baseline.
  void compareImage(QWidget* widget,
                    const QString& baseline,
                    double threshold,
                    const QString& tempDir);

 // Compare the image (for now, only png files are supported)
 // with the baseline.
 void compareImage(const QString& pngfile,
                   const QString& baseline,
                   double threshold,
                   const QString& tempDir);
protected:
  virtual void run();

  void compareImageInternal(vtkImageData* vtkimage,
    const QString& baseline, double threshold, const QString& tempDir);
protected slots:
  void doComparison();
};

#endif // !_pqPythonEventSourceImage_h

