// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqBackgroundEditorWidget_h
#define pqBackgroundEditorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyGroupWidget.h"

class vtkSMProxy;
class vtkSMPropertyGroup;

class PQAPPLICATIONCOMPONENTS_EXPORT pqBackgroundEditorWidget : public pqPropertyGroupWidget
{
public:
  pqBackgroundEditorWidget(vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup,
    QWidget* parentObject = nullptr, bool forEnvironment = false);
  ~pqBackgroundEditorWidget() override;

  bool gradientBackground() const;
  void setGradientBackground(bool gradientBackground);
  bool imageBackground() const;
  void setImageBackground(bool imageBackground);
  bool skyboxBackground() const;
  void setSkyboxBackground(bool skyboxBackground);
  bool environmentLighting() const;
  void setEnvironmentLighting(bool envLighting);

Q_SIGNALS:
  void gradientBackgroundChanged();
  void imageBackgroundChanged();
  void skyboxBackgroundChanged();
  void environmentLightingChanged();

protected Q_SLOTS:
  void currentIndexChangedBackgroundType(int type);
  void clickedRestoreDefaultColor();
  void clickedRestoreDefaultColor2();

private:
  typedef pqPropertyGroupWidget Superclass;

  void changeColor(const char* propertyName);
  void fireGradientAndImageChanged(int oldType, int newType);

  Q_OBJECT
  Q_PROPERTY(bool gradientBackground READ gradientBackground WRITE setGradientBackground)
  Q_PROPERTY(bool imageBackground READ imageBackground WRITE setImageBackground)
  Q_PROPERTY(bool skyboxBackground READ skyboxBackground WRITE setSkyboxBackground)
  Q_PROPERTY(bool environmentLighting READ environmentLighting WRITE setEnvironmentLighting)

  class pqInternal;
  pqInternal* Internal;
};

#endif
