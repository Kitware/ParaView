/*=========================================================================

   Program: ParaView
   Module:    pqActiveChartOptions.h

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

/// \file pqActiveChartOptions.h
/// \date 7/27/2007

#ifndef _pqActiveChartOptions_h
#define _pqActiveChartOptions_h


#include "pqComponentsExport.h"
#include "pqActiveViewOptions.h"

class pqActiveChartOptionsInternal;
class pqOptionsDialog;


class PQCOMPONENTS_EXPORT pqActiveChartOptions : public pqActiveViewOptions
{
  Q_OBJECT

public:
  pqActiveChartOptions(QObject *parent=0);
  virtual ~pqActiveChartOptions();

  virtual void showOptions(pqView *view, QWidget *parent=0);
  virtual void changeView(pqView *view);
  virtual void closeOptions();

private slots:
  void finishDialog(int result);
  void cleanupDialog();

  void setTitleModified();
  void setTitleFontModified();
  void setTitleColorModified();
  void setTitleAlignmentModified();
  void setShowLegendModified();
  void setLegendLocationModified();
  void setLegendFlowModified();
  void setShowAxisModified();
  void setShowAxisGridModified();
  void setAxisGridTypeModified();
  void setAxisColorModified();
  void setAxisGridColorModified();
  void setShowAxisLabelsModified();
  void setAxisLabelFontModified();
  void setAxisLabelColorModified();
  void setAxisLabelNotationModified();
  void setAxisLabelPrecisionModified();
  void setAxisScaleModified();
  void setAxisBehaviorModified();
  void setAxisMinimumModified();
  void setAxisMaximumModified();
  void setAxisLabelsModified();
  void setAxisTitleModified();
  void setAxisTitleFontModified();
  void setAxisTitleColorModified();
  void setAxisTitleAlignmentModified();

private:
  void initializeOptions();

private:
  pqActiveChartOptionsInternal *Internal;
  pqOptionsDialog *Dialog;
};

#endif
