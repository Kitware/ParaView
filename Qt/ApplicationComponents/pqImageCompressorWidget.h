/*=========================================================================

   Program: ParaView
   Module:  pqImageCompressorWidget.h

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
#ifndef pqImageCompressorWidget_h
#define pqImageCompressorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

/**
* pqImageCompressorWidget is a pqPropertyWidget designed to be used
* for "CompressorConfig" property on "RenderView" or  "RenderViewSettings"
* proxy. It works with a string property that expects the config in
* a predetermined format (refer to the code for details on the format).
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqImageCompressorWidget : public pqPropertyWidget
{
  Q_OBJECT;
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QString compressorConfig READ compressorConfig WRITE setCompressorConfig);

public:
  pqImageCompressorWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = 0);
  ~pqImageCompressorWidget() override;

  QString compressorConfig() const;

public Q_SLOTS:
  void setCompressorConfig(const QString&);

Q_SIGNALS:
  void compressorConfigChanged();
private Q_SLOTS:
  void currentIndexChanged(int);
  void setConfigurationDefault(int);

private:
  Q_DISABLE_COPY(pqImageCompressorWidget)
  class pqInternals;
  pqInternals* Internals;
};

#endif
