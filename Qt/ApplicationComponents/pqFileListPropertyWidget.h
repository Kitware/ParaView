/*=========================================================================

   Program: ParaView
   Module:  pqFileListPropertyWidget.h

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
#ifndef pqFileListPropertyWidget_h
#define pqFileListPropertyWidget_h

#include "pqApplicationComponentsModule.h" // for exports
#include "pqPropertyWidget.h"

#include <memory> // for std::unique_ptr

/**
 * @class pqFileListPropertyWidget
 * @brief list widget for file selection
 *
 * pqFileListPropertyWidget is intended to be used with properties that allow
 * users to select files (or directories). Instead of showing a single input
 * widget, which is the case by default (see `pqStringVectorPropertyWidget`),
 * this custom widget shows a list (rather a table) with all the selected
 * filenames. Users can add/remove files.
 *
 * To use this widget on your string-vector property, set the panel widget to
 * "file_list".
 *
 * For example:
 *
 * @code{xml}
 *
 * <SourceProxy name="TestSource" >
 *   <StringVectorProperty name="Files"
 *         command="..."
 *         default_values="/tmp/test.txt"
 *         number_of_elements="1"
 *         repeatable="1"
 *         number_of_elements_per_command="1"
 *         panel_widget="file_list">
 *         <FileListDomain name="files"/>
 *   </StringVectorProperty>
 * </SourceProxy>
 *
 * @endcode
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqFileListPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QStringList fileNames READ fileNames WRITE setFileNames NOTIFY fileNamesChanged);

public:
  explicit pqFileListPropertyWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqFileListPropertyWidget() override;

  ///@{
  /**
   * Get/Set the filenames.
   */
  const QStringList& fileNames() const;
  void setFileNames(const QStringList& filenames);
  ///@}

Q_SIGNALS:
  /**
   * Signal fired when filenames are changed.
   */
  void fileNamesChanged();

private:
  Q_DISABLE_COPY(pqFileListPropertyWidget);
  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
  mutable QStringList FileNamesCache;
};

#endif
