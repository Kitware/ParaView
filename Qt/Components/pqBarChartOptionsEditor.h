/*=========================================================================

   Program: ParaView
   Module:    pqBarChartOptionsEditor.h

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

#ifndef _pqBarChartOptionsEditor_h
#define _pqBarChartOptionsEditor_h

#include "pqComponentsExport.h"
#include "pqOptionsPage.h"
#include "vtkQtBarChartOptions.h" // needed for enum

class pqBarChartOptionsEditorForm;
class QString;


class PQCOMPONENTS_EXPORT pqBarChartOptionsEditor : public pqOptionsPage
{
  Q_OBJECT

public:
  /// \brief
  ///   Constructs a bar chart options page.
  /// \param parent The parent widget.
  pqBarChartOptionsEditor(QWidget *parent=0);
  virtual ~pqBarChartOptionsEditor();

  void getHelpFormat(QString &format) const;
  void setHelpFormat(const QString &format);

  vtkQtBarChartOptions::OutlineStyle getOutlineStyle() const;
  void setOutlineStyle(vtkQtBarChartOptions::OutlineStyle outline);

  float getBarGroupFraction() const;
  void setBarGroupFraction(float fraction);

  float getBarWidthFraction() const;
  void setBarWidthFraction(float fraction);

signals:
  void helpFormatChanged(const QString &format);
  void outlineStyleChanged(vtkQtBarChartOptions::OutlineStyle outline);
  void barGroupFractionChanged(float fraction);
  void barWidthFractionChanged(float fraction);

private slots:
  void convertOutlineStyle(int index);
  void convertGroupFraction(double fraction);
  void convertWidthFraction(double fraction);

private:
  pqBarChartOptionsEditorForm *Form; ///< Stores the UI data.
};

#endif
