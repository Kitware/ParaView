/*=========================================================================

   Program: ParaView
   Module:    pqStackedChartOptionsEditor.h

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

========================================================================*/

#ifndef _pqStackedChartOptionsEditor_h
#define _pqStackedChartOptionsEditor_h

#include "pqComponentsExport.h"
#include "pqOptionsPage.h"

class pqStackedChartOptionsEditorForm;
class QString;


class PQCOMPONENTS_EXPORT pqStackedChartOptionsEditor : public pqOptionsPage
{
  Q_OBJECT

public:
  /// \brief
  ///   Constructs a line chart options page.
  /// \param parent The parent widget.
  pqStackedChartOptionsEditor(QWidget *parent=0);
  virtual ~pqStackedChartOptionsEditor();

  void getHelpFormat(QString &format) const;
  void setHelpFormat(const QString &format);

  bool isSumNormalized() const;
  void setSumNormalized(bool normalized);

  bool isGradientDisplayed() const;
  void setGradientDisplayed(bool gradient);

signals:
  void helpFormatChanged(const QString &format);
  void normalizationChanged(bool normalized);
  void gradientChanged(bool displayed);

private:
  pqStackedChartOptionsEditorForm *Form; ///< Stores the UI data.
};

#endif
