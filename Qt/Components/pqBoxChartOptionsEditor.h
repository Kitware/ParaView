/*=========================================================================

   Program: ParaView
   Module:    pqBoxChartOptionsEditor.h

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

#ifndef _pqBoxChartOptionsEditor_h
#define _pqBoxChartOptionsEditor_h

#include "pqComponentsExport.h"
#include "pqOptionsPage.h"
#include "vtkQtStatisticalBoxChartOptions.h" // needed for enum

class pqBoxChartOptionsEditorForm;
class QString;


class PQCOMPONENTS_EXPORT pqBoxChartOptionsEditor : public pqOptionsPage
{
  Q_OBJECT

public:
  /// \brief
  ///   Constructs a box chart options page.
  /// \param parent The parent widget.
  pqBoxChartOptionsEditor(QWidget *parent=0);
  virtual ~pqBoxChartOptionsEditor();

  void getHelpFormat(QString &format) const;
  void setHelpFormat(const QString &format);

  void getOutlierFormat(QString &format) const;
  void setOutlierFormat(const QString &format);

  vtkQtStatisticalBoxChartOptions::OutlineStyle getOutlineStyle() const;
  void setOutlineStyle(vtkQtStatisticalBoxChartOptions::OutlineStyle outline);

  float getBoxWidthFraction() const;
  void setBoxWidthFraction(float fraction);

signals:
  void helpFormatChanged(const QString &format);
  void outlierFormatChanged(const QString &format);
  void outlineStyleChanged(vtkQtStatisticalBoxChartOptions::OutlineStyle outline);
  void boxWidthFractionChanged(float fraction);

private slots:
  void convertOutlineStyle(int index);
  void convertWidthFraction(double fraction);

private:
  pqBoxChartOptionsEditorForm *Form; ///< Stores the UI data.
};

#endif
