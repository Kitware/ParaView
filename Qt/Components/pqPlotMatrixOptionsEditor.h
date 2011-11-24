/*=========================================================================

   Program: ParaView
   Module:    pqPlotMatrixOptionsEditor.h

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

/// \file pqPlotMatrixOptionsEditor.h
/// \date 7/20/2007

#ifndef _pqPlotMatrixOptionsEditor_h
#define _pqPlotMatrixOptionsEditor_h

#include "pqComponentsExport.h"
#include "pqOptionsContainer.h"
#include "pqChartValue.h"

class pqPlotMatrixOptionsEditorForm;
class pqView;
class QColor;
class QFont;
class QLabel;
class QString;
class QStringList;
class vtkSMProxy;

/// \class pqPlotMatrixOptionsEditor
/// \brief
///   The pqPlotMatrixOptionsEditor class is the user interface for setting
///   the plot matrix view options.
class PQCOMPONENTS_EXPORT pqPlotMatrixOptionsEditor : public pqOptionsContainer
{
  Q_OBJECT

public:
  pqPlotMatrixOptionsEditor(QWidget *parent=0);
  virtual ~pqPlotMatrixOptionsEditor();

  // set the view to show options for
  void setView(pqView* view);
  pqView* getView();

  // set the current page
  virtual void setPage(const QString &page);
  // return a list of strings for pages we have
  virtual QStringList getPageList();

  // apply the changes
  virtual void applyChanges();
  // reset the changes
  virtual void resetChanges();

  // tell pqOptionsDialog that we want an apply button
  virtual bool isApplyUsed() const { return true; }

protected slots:
  void connectGUI();

  // Setters for the axis elements of the form
  void setGridVisibility(bool visible);
  void setChartBackgroundColor(const QColor& color);
  void setAxisColor(const QColor& color);
  void setGridColor(const QColor& color);
  void setLabelVisibility(bool visible);
  void pickLabelFont();
  void setAxisLabelColor(const QColor& color);
  void setLabelNotation(int notation);
  void setLabelPrecision(int precision);
  void setToolTipNotation(int notation);
  void setToolTipPrecision(int precision);

  void pickTitleFont();

private:
  void updateOptions();
  void applyChartOptions();
  void loadChartPage();
  bool pickFont(QLabel *label, QFont &font);
  void updateDescription(QLabel *label, const QFont &newFont);
  vtkSMProxy* getProxy();
  class pqInternal;
  pqInternal* Internal;
};

#endif
