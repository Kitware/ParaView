/*=========================================================================

   Program: ParaView
   Module:  pqPresetToPixmap.h

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
#ifndef pqPresetToPixmap_h
#define pqPresetToPixmap_h

#include "pqComponentsModule.h"
#include <QObject>
#include <QScopedPointer>
#include <vtk_jsoncpp_fwd.h> // for forward declarations

class vtkPiecewiseFunction;
class vtkScalarsToColors;
class QPixmap;
class QSize;

/**
* pqPresetToPixmap is a helper class to generate QPixmap from a color/opacity
* preset. Use pqPresetToPixmap::render() to generate a QPixmap for a transfer
* function.
*/
class PQCOMPONENTS_EXPORT pqPresetToPixmap : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPresetToPixmap(QObject* parent = 0);
  virtual ~pqPresetToPixmap();

  /**
  * Render a preset to a pixmap for the given resolution.
  */
  QPixmap render(const Json::Value& preset, const QSize& resolution) const;

protected:
  /**
  * Renders a color transfer function preset.
  */
  QPixmap renderColorTransferFunction(
    vtkScalarsToColors* stc, vtkPiecewiseFunction* pf, const QSize& resolution) const;

  /**
  * Renders a color transfer function preset.
  */
  QPixmap renderIndexedColorTransferFunction(
    vtkScalarsToColors* stc, const QSize& resolution) const;

private:
  Q_DISABLE_COPY(pqPresetToPixmap)
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
