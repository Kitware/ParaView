/*=========================================================================

   Program: ParaView
   Module:    pqSampleScalarAddRangeDialog.h

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
#ifndef pqSampleScalarAddRangeDialog_h
#define pqSampleScalarAddRangeDialog_h

#include "pqComponentsModule.h"
#include "vtkSetGet.h" // for VTK_LEGACY_REMOVE

#if !defined(VTK_LEGACY_REMOVE)
#include <QDialog>
#include <QVariant>

/**
 * @class pqSampleScalarAddRangeDialog
 *
 * @deprecated in ParaView 5.8. Use `pqSeriesGeneratorDialog` instead.
 *
 * Provides a dialog for specifying a range of scalar samples, used
 * by pqSampleScalarWidget
*/
class PQCOMPONENTS_EXPORT pqSampleScalarAddRangeDialog : public QDialog
{
  typedef QDialog Superclass;

  Q_OBJECT

public:
  pqSampleScalarAddRangeDialog(
    double from, double to, unsigned long steps, bool logarithmic, QWidget* parent = 0);
  ~pqSampleScalarAddRangeDialog() override;

  double from() const;
  void setFrom(double value);
  double to() const;
  void setTo(double value);
  unsigned long steps() const;
  void setSteps(unsigned long number);
  bool logarithmic() const;
  void setLogarithmic(bool useLog);

  void setLogRangeStrict(bool on);
  bool isLogRangeStrict() const;
  QVariantList getRange() const;

private Q_SLOTS:
  void onRangeChanged();

private:
  pqSampleScalarAddRangeDialog(const pqSampleScalarAddRangeDialog&);
  pqSampleScalarAddRangeDialog& operator=(const pqSampleScalarAddRangeDialog&);

  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !defined(VTK_LEGACY_REMOVE)
#endif // !_pqSampleScalarAddRangeDialog_h
