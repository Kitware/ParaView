/*=========================================================================

   Program: ParaView
   Module:    pqRescaleRange.h

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

/**
* \file pqRescaleRange.h
* \date 3/28/2007
*/

#ifndef _pqRescaleRange_h
#define _pqRescaleRange_h

#include "pqComponentsModule.h"
#include <QDialog>

class pqRescaleRangeForm;

class PQCOMPONENTS_EXPORT pqRescaleRange : public QDialog
{
  Q_OBJECT
public:
  pqRescaleRange(QWidget* parent = 0);
  ~pqRescaleRange() override;

  // Get the minimum of the color map range
  double minimum() const;

  // Get the maximum of the color map range
  double maximum() const;

  // Show or hide the opacity controls
  void showOpacityControls(bool show);

  // Get the minimum of the opacity map range
  double opacityMinimum() const;

  // Get the maximum of the opacity map range
  double opacityMaximum() const;

  // Initialize the range for the color map
  void setRange(double min, double max);

  // Initialize the range for a separate opacity map, if the option is available
  void setOpacityRange(double min, double max);

  bool lock() const { return this->Lock; }

protected Q_SLOTS:
  void validate();
  void rescaleAndLock();

protected:
  pqRescaleRangeForm* Form;
  bool Lock;
};

#endif
