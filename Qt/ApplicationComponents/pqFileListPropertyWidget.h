// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
