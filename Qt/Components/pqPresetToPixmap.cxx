/*=========================================================================

   Program: ParaView
   Module:  pqPresetToPixmap.cxx

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
#include "pqPresetToPixmap.h"

#include "pqActiveObjects.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPVDiscretizableColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPixmap>
#include <QSize>

class pqPresetToPixmap::pqInternals
{
  vtkSmartPointer<vtkSMProxy> PVLookupTable;
  vtkSmartPointer<vtkSMProxy> PiecewiseFunction;

public:
  pqInternals()
  {
    vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
    this->PiecewiseFunction.TakeReference(
      pxm->NewProxy("piecewise_functions", "PiecewiseFunction"));
    this->PiecewiseFunction->SetLocation(vtkSMSession::CLIENT);
    this->PiecewiseFunction->UpdateVTKObjects();

    this->PVLookupTable.TakeReference(pxm->NewProxy("lookup_tables", "PVLookupTable"));
    this->PVLookupTable->SetLocation(vtkSMSession::CLIENT);
    vtkSMPropertyHelper(this->PVLookupTable, "ScalarOpacityFunction").Set(this->PiecewiseFunction);
  }

  vtkSMProxy* lookupTable() const
  {
    this->PVLookupTable->ResetPropertiesToXMLDefaults();
    this->PVLookupTable->UpdateVTKObjects();
    return this->PVLookupTable;
  }

  vtkSMProxy* piecewiseFunction() const
  {
    this->PiecewiseFunction->ResetPropertiesToXMLDefaults();
    this->PiecewiseFunction->UpdateVTKObjects();
    return this->PiecewiseFunction;
  }

  bool loadPreset(vtkSMProxy* proxy, const Json::Value& preset)
  {
    return vtkSMTransferFunctionProxy::ApplyPreset(proxy, preset);
  }
};

//-----------------------------------------------------------------------------
pqPresetToPixmap::pqPresetToPixmap(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqPresetToPixmap::pqInternals())
{
}

//-----------------------------------------------------------------------------
pqPresetToPixmap::~pqPresetToPixmap()
{
}

//-----------------------------------------------------------------------------
QPixmap pqPresetToPixmap::render(const Json::Value& preset, const QSize& resolution) const
{
  if (resolution.width() <= 0 || resolution.height() <= 0)
  {
    return QPixmap();
  }

  pqInternals& internals = (*this->Internals);
  vtkSMProxy* lutProxy = internals.lookupTable();
  vtkSMTransferFunctionProxy::ApplyPreset(lutProxy, preset);
  vtkSMTransferFunctionProxy::RescaleTransferFunction(lutProxy, 1, 100, false);
  vtkScalarsToColors* stc = vtkScalarsToColors::SafeDownCast(lutProxy->GetClientSideObject());
  if (stc->GetIndexedLookup())
  {
    return this->renderIndexedColorTransferFunction(stc, resolution);
  }
  else
  {
    vtkPiecewiseFunction* pf = NULL;
    auto presets = vtkSMTransferFunctionPresets::GetInstance();
    if (presets->GetPresetHasOpacities(preset))
    {
      vtkSMProxy* piecewiseFunctionProxy = internals.piecewiseFunction();
      vtkSMTransferFunctionProxy::ApplyPreset(piecewiseFunctionProxy, preset);
      vtkSMTransferFunctionProxy::RescaleTransferFunction(piecewiseFunctionProxy, 1, 100, false);
      pf = vtkPiecewiseFunction::SafeDownCast(piecewiseFunctionProxy->GetClientSideObject());
    }
    return this->renderColorTransferFunction(stc, pf, resolution);
  }
}

//-----------------------------------------------------------------------------
QPixmap pqPresetToPixmap::renderColorTransferFunction(
  vtkScalarsToColors* stc, vtkPiecewiseFunction* pf, const QSize& resolution) const
{
  int numSamples = std::min(256, std::max(2, resolution.width()));
  vtkNew<vtkFloatArray> data;
  data->SetNumberOfTuples(numSamples);
  const double* range = stc->GetRange();
  int isUsingLog = stc->UsingLogScale();
  double lrange[2];
  if (isUsingLog)
  {
    lrange[0] = log10(range[0]);
    lrange[1] = log10(range[1]);
  }
  for (vtkIdType cc = 0, max = numSamples; cc < max; ++cc)
  {
    double normVal = static_cast<double>(cc) / (max - 1);
    double val;
    if (isUsingLog)
    {
      double lval = lrange[0] + normVal * (lrange[1] - lrange[0]);
      val = pow(10.0, lval);
    }
    else
    {
      val = (range[1] - range[0]) * normVal + range[0];
    }
    data->SetValue(cc, val);
  }
  vtkSmartPointer<vtkUnsignedCharArray> colors;
  colors.TakeReference(vtkUnsignedCharArray::SafeDownCast(
    stc->MapScalars(data.GetPointer(), VTK_COLOR_MODE_MAP_SCALARS, 0)));
  QImage image(numSamples, 1, QImage::Format_RGB888);
  for (int cc = 0; cc < numSamples; ++cc)
  {
    unsigned char* ptr = colors->GetPointer(4 * cc);
    image.setPixel(cc, 0, qRgb(ptr[0], ptr[1], ptr[2]));
  }
  if (pf)
  {
    image = image.scaled(image.width(), resolution.height());
    QPixmap pixmap = QPixmap::fromImage(image);
    QPainterPath path;
    path.moveTo(0, 0);
    for (vtkIdType cc = 0, max = numSamples; cc < max; cc += 10)
    {
      float x = data->GetValue(cc);
      int y = static_cast<int>(resolution.height() * (1.0 - pf->GetValue(x)));
      path.lineTo(cc, y);
    }
    QPainter painter(&pixmap);
    painter.setPen(Qt::black);
    // painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen = painter.pen();
    pen.setWidth(2);
    painter.strokePath(path, pen);
    return pixmap;
  }
  else
  {
    image = image.scaled(resolution);
    return QPixmap::fromImage(image);
  }
}

//-----------------------------------------------------------------------------
// This is brought over directly from pqColorMapModel::generateCategoricalPreview().
//-----------------------------------------------------------------------------

// The smallest number of pixels along an edge that a color swatch should be.
#define PQ_MIN_SWATCH_DIM 6
// The amount of padding between swatches and the edge of the palette and/or a neighbor swatch
// [pixels].
#define PQ_SWATCH_PAD 2
// The number of pixels each swatch's insert border color should be drawn.
#define PQ_SWATCH_BORDER 1
QPixmap pqPresetToPixmap::renderIndexedColorTransferFunction(
  vtkScalarsToColors* stc, const QSize& size) const
{
  vtkPVDiscretizableColorTransferFunction* dct =
    vtkPVDiscretizableColorTransferFunction::SafeDownCast(stc);
  int numSwatches = static_cast<int>(dct->GetNumberOfIndexedColorsInFullSet());
  if (numSwatches < 1)
  {
    return QPixmap();
  }

  // This is needed since the vtkDiscretizableColorTransferFunction only
  // respects categorical colors for which annotations are present (currently)
  dct->ResetAnnotationsInFullSet();
  for (int cc = 0; cc < numSwatches; cc++)
  {
    dct->SetAnnotationInFullSet(vtkVariant(cc), "");
  }
  dct->Build();

  // Create a pixmap and painter.
  QPixmap palette(size);
  QPainter painter(&palette);

  int wmp = size.width() - PQ_SWATCH_PAD, hmp = size.height() - PQ_SWATCH_PAD;

  // I. Determine the maximum number of rows and columns of swatches
  // Maximum if we constrain height:
  int Nvmax = hmp / (PQ_MIN_SWATCH_DIM + PQ_SWATCH_PAD);
  // Maximum if we don't constrain height:
  int NvUnconstrained = static_cast<int>(ceil(sqrt(numSwatches)));

  // II. Determine the actual number of rows and columns
  int N = numSwatches, Nh = N, Nv = 1;
  while (((wmp / Nh < PQ_MIN_SWATCH_DIM + PQ_SWATCH_PAD) ||
           // aspect ratio < 2/3 (integer math) and we have headroom.
           ((hmp * Nh * 10) / (Nv * wmp) > 15 && Nv < Nvmax)) &&
    Nv < NvUnconstrained)
  {
    ++Nv;
    // Now determine the value for Nh that makes the swatch
    // aspect ratio as near to square as possible:
    double bestQ = vtkMath::Inf();
    int best = -1;
    int searchNh;
    for (searchNh = Nh; searchNh * Nv >= N; --searchNh)
    {
      double ar = Nv * wmp / static_cast<double>(hmp * searchNh);
      double q = (ar >= 1.0) ? ar : 1. / ar;
      if (q < bestQ)
      {
        bestQ = q;
        best = searchNh;
      }
    }
    Nh = best;
  }

  // III. Determine swatch size and number of swatches that can actually be displayed
  int ws = wmp / Nh - PQ_SWATCH_PAD;
  int hs = hmp / Nv - PQ_SWATCH_PAD;
  if (ws < 0)
    ws = PQ_MIN_SWATCH_DIM;
  if (hs < 0)
    hs = PQ_MIN_SWATCH_DIM;
  int Nd =
    Nh * Nv; // This may be more or less than N, but no more than this many swatches will be drawn.
  int ss = ws < hs ? ws : hs; // Force aspect ratio to 1 and then update Nh, Nv, Nd
  Nh = wmp / (ss + PQ_SWATCH_PAD);
  Nv = hmp / (ss + PQ_SWATCH_PAD);
  Nd = Nh * Nv;
  int elideLf = (Nd < N ? Nd / 2 : N - 1);
  int elideRt = (Nd < N ? N - Nd / 2 + 1 : N);
  Nd = Nd > N ? N : Nd;

  // IV. Clear to background.
  QPen blank(Qt::NoPen);
  painter.setBrush(QColor("white"));
  painter.setPen(blank);
  painter.drawRect(0, 0, size.width(), size.height());

  // IV. Draw swatches.
  QPen outline(Qt::SolidLine);
  outline.setWidth(PQ_SWATCH_BORDER);
  painter.setPen(outline);
  int row, col, swatch;
  for (row = 0, col = 0, swatch = 0; swatch <= elideLf; ++swatch)
  {
    double drgba[4];
    dct->GetIndexedColorInFullSet(swatch, drgba);
    painter.setBrush(QColor::fromRgbF(drgba[0], drgba[1], drgba[2]));
    painter.drawRect(PQ_SWATCH_PAD + col * (PQ_SWATCH_PAD + ss) + 0.5,
      PQ_SWATCH_PAD + row * (PQ_SWATCH_PAD + ss) + 0.5, ss, ss);

    if (++col == Nh)
    {
      col = 0;
      ++row;
    }
  }

  // Advance one entry over the elided swatch
  if (++col == Nh)
  {
    col = 0;
    ++row;
  }
  ++swatch;

  // Now pick up and draw the remaining swatches, if any
  int entry = elideRt;
  for (; swatch < Nd; ++swatch, ++entry)
  {
    double drgba[4];
    dct->GetIndexedColorInFullSet(swatch, drgba);
    painter.setBrush(QColor::fromRgbF(drgba[0], drgba[1], drgba[2]));
    painter.drawRect(PQ_SWATCH_PAD + col * (PQ_SWATCH_PAD + ss) + 0.5,
      PQ_SWATCH_PAD + row * (PQ_SWATCH_PAD + ss) + 0.5, ss, ss);

    if (++col == Nh)
    {
      col = 0;
      ++row;
    }
  }

  return palette;
}
