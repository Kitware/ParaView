/*=========================================================================

   Program:   ParaView
   Module:    pqDoubleRangeWidget.h

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
#ifndef pqDoubleRangeWidget_h
#define pqDoubleRangeWidget_h

#include "pqDoubleSliderWidget.h"
#include "pqWidgetsModule.h"
#include "vtkSetGet.h" // for VTK_LEGACY
#include <QWidget>

/**
 * Extends pqDoubleSliderWidget to use it with a range of doubles : provides
 * control on min/max, resolution and on line edit validator.
 */
class PQWIDGETS_EXPORT pqDoubleRangeWidget : public pqDoubleSliderWidget
{
  Q_OBJECT
  Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
  Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
#if !defined(VTK_LEGACY_REMOVE)
  Q_PROPERTY(bool strictRange READ strictRange WRITE setStrictRange);
#endif
  Q_PROPERTY(int resolution READ resolution WRITE setResolution)

  typedef pqDoubleSliderWidget Superclass;

public:
  /**
  * constructor requires the proxy, property
  */
  pqDoubleRangeWidget(QWidget* parent = NULL);
  ~pqDoubleRangeWidget() override;

  // get the min range value
  double minimum() const;
  // get the max range value
  double maximum() const;

  /**
   * @deprecated strictRange is deprecated and is not working as intended
   */
  VTK_LEGACY(bool strictRange() const);

  // returns the resolution.
  int resolution() const;

public Q_SLOTS:
  // set the min range value
  void setMinimum(double);
  // set the max range value
  void setMaximum(double);

#if !defined(VTK_LEGACY_REMOVE)
  /**
   * @deprecated strictRange is deprecated and is not working as intended
   */
  void setStrictRange(bool);
#endif

  // set the resolution.
  void setResolution(int);

protected:
  int valueToSliderPos(double val) override;
  double sliderPosToValue(int pos) override;

private Q_SLOTS:
  void updateValidator();

private:
  int Resolution;
  double Minimum;
  double Maximum;
#if !defined(VTK_LEGACY_REMOVE)
  bool StrictRange = false;
#endif
};

#endif
