// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqImageCompressorWidget_h
#define pqImageCompressorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

/**
 * pqImageCompressorWidget is a pqPropertyWidget designed to be used
 * for "CompressorConfig" property on "RenderView" or  "RenderViewSettings"
 * proxy. It works with a string property that expects the config in
 * a predetermined format (refer to the code for details on the format).
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqImageCompressorWidget : public pqPropertyWidget
{
  Q_OBJECT;
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QString compressorConfig READ compressorConfig WRITE setCompressorConfig);

public:
  pqImageCompressorWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqImageCompressorWidget() override;

  QString compressorConfig() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setCompressorConfig(const QString&);

Q_SIGNALS:
  void compressorConfigChanged();
private Q_SLOTS:
  void currentIndexChanged(int);
  void setConfigurationDefault(int);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqImageCompressorWidget)
  class pqInternals;
  pqInternals* Internals;
};

#endif
