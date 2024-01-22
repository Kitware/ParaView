// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqStringVectorPropertyWidget_h
#define pqStringVectorPropertyWidget_h

#include "pqPropertyLinks.h"
#include "pqPropertyWidget.h"

class vtkSMStringVectorProperty;
class PQCOMPONENTS_EXPORT pqStringVectorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
public:
  pqStringVectorPropertyWidget(
    vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = nullptr);
  ~pqStringVectorPropertyWidget() override;

  /**
   * Factory method to instantiate a hard-coded type of pqPropertyWidget
   * subclass for t he vtkSMStringVectorProperty.
   */
  static pqPropertyWidget* createWidget(
    vtkSMStringVectorProperty* smproperty, vtkSMProxy* smproxy, QWidget* parent = nullptr);

  /**
   * Method to process file-choice related hints.
   */
  static void processFileChooserHints(vtkPVXMLElement* hints, bool& directoryMode, bool& anyFile,
    QString& filter, bool& browseLocalFileSystem);

private Q_SLOTS:
  /**
   * Show a warning box on property change if specified by an hint.
   */
  void showWarningOnChange();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqStringVectorPropertyWidget);

  bool widgetHintHasAttributeEqualTo(const std::string& attribute, const std::string& value);

  vtkPVXMLElement* WidgetHint = nullptr;
  vtkPVXMLElement* WarnOnChangeHint = nullptr;
  bool WarningTriggered = false;
};

#endif // pqStringVectorPropertyWidget_h
