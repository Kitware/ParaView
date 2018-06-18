/*=========================================================================

   Program: ParaView
   Module:  pqExportViewSelection.h

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
#ifndef pqExportViewSelection_h
#define pqExportViewSelection_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqRenderViewBase;
class pqContextView;
class pqImageOutputInfo;

namespace Ui
{
class ExportViewSelection;
}

/**
* @brief Widget to select views to be included in a coprocessing script.
*
* The user selection can be queried as a string to be included in a Python
* script directly. The widget is used by pqCinemaConfiguration and
* pqSGExportStateWizard.
*/
class PQCOMPONENTS_EXPORT pqExportViewSelection : public QWidget
{
  Q_OBJECT;

public:
  pqExportViewSelection(QWidget* parent_ = NULL);
  ~pqExportViewSelection() override;

  void populateViews(
    QList<pqRenderViewBase*> const& renderViews, QList<pqContextView*> const& contextViews);

  /**
  * Hides/shows cinema options.
  */
  void setCinemaVisible(bool status);

  /// Hides/shows catalyst specific entries
  void setCatalystOptionsVisible(bool status);

  QList<pqImageOutputInfo*> getImageOutputInfos();

  /**
  * Returns a string containing a comma separated set of views with each
  * view defined as in 'format'.
  * Order of view values:
  * 1. View name
  * 2. Image file name
  * 3. Frequency
  * 4. Fit to screen
  * 5. Magnification
  * 6. Image width
  * 7. Image height
  * 8. Cinema specific options (dictionary; phi, theta, composite, etc..)
  *
  * Example: Format as defined in pqCinemaConfiguration
  * format = "'%1' : ['%2', %3, %4, %5, %6, %7, %8]"
  * returns -> 'ViewName1' : ['Imname', 1, 1, 1, 1, 1, {'composite': True ...}],
  *            'ViewName2' : [...],
  *            ... (for N views)
  */
  QString getSelectionAsString(QString const& scriptFormat);

signals:
  void arraySelectionEnabledChanged(bool enabled);

private slots:

  void onPreviousClicked();
  void onNextClicked();

private:
  /**
  * Checks if a given string marker exists in infoFormat and replaces it with the
  * appropriate value in parameters.
  * @note This helper for getSelectionAsPythonScript(...) is needed given that QString::arg's
  * result is undefined when called on a string with no unreplaced "%i" markers. See
  * , http://doc.qt.io/qt-4.8/qstring.html#arg
  */
  void patchFormatString(QMap<QString, QString> const& parameters, QString& infoFormat);

  /**
  * @note This is templated to handle pqRenderViewBase* and pqContextView*
  * given that only these two pqView sub-types are supported by cinema. When
  * cinema gives support to all the pqView types the template will be removed.
  */
  template <typename T>
  void addViews(T const& views, int numberOfViews);

  /**
  *
  */

  Ui::ExportViewSelection* Ui;
};

#endif
