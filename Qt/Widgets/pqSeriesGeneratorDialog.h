/*=========================================================================

   Program: ParaView
   Module:  pqSeriesGeneratorDialog.h

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
#ifndef pqSeriesGeneratorDialog_h
#define pqSeriesGeneratorDialog_h

#include "pqWidgetsModule.h" // for exports
#include <QDialog>
#include <QScopedPointer> // for ivar

/**
 * @class pqSeriesGeneratorDialog
 * @brief dialog to generate a number series
 *
 * pqSeriesGeneratorDialog is a simple dialog that lets the user generate a
 * series of numbers. Multiple series are supported including linear,
 * logarithmic and geometric.
 *
 */
class PQWIDGETS_EXPORT pqSeriesGeneratorDialog : public QDialog
{
  Q_OBJECT
  using Superclass = QDialog;

public:
  pqSeriesGeneratorDialog(
    double min, double max, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  ~pqSeriesGeneratorDialog() override;

  /**
   * Returns the generated number series.
   */
  QVector<double> series() const;

private:
  Q_DISABLE_COPY(pqSeriesGeneratorDialog);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
