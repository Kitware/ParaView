// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqKeyFrameTypeWidget_h
#define pqKeyFrameTypeWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

#include <vtk_jsoncpp_fwd.h> // for forward declarations

class QComboBox;

class PQCOMPONENTS_EXPORT pqKeyFrameTypeWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString type READ type WRITE setType)

  Q_PROPERTY(QString base READ base WRITE setBase)
  Q_PROPERTY(QString startPower READ startPower WRITE setStartPower)
  Q_PROPERTY(QString endPower READ endPower WRITE setEndPower)

  Q_PROPERTY(double phase READ phase WRITE setPhase)
  Q_PROPERTY(QString offset READ offset WRITE setOffset)
  Q_PROPERTY(QString frequency READ frequency WRITE setFrequency)

public:
  pqKeyFrameTypeWidget(QWidget* parent = nullptr);
  ~pqKeyFrameTypeWidget() override;

  /**
   * Initialize the widget properties using JSON.
   */
  void initializeUsingJSON(const Json::Value& json);

  /**
   * Generate a JSON representing the widget configuration.
   */
  Json::Value serializeToJSON() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setType(const QString& text);
  void setBase(const QString& text);
  void setStartPower(const QString& text);
  void setEndPower(const QString& text);
  void setPhase(double);
  void setOffset(const QString& text);
  void setFrequency(const QString& text);

public: // NOLINT(readability-redundant-access-specifiers)
  QString type() const;
  QString base() const;
  QString startPower() const;
  QString endPower() const;
  double phase() const;
  QString offset() const;
  QString frequency() const;

  QComboBox* typeComboBox() const;

Q_SIGNALS:
  void typeChanged(const QString&);
  void baseChanged(const QString&);
  void startPowerChanged(const QString&);
  void endPowerChanged(const QString&);
  void phaseChanged(double);
  void offsetChanged(const QString&);
  void frequencyChanged(const QString&);

protected Q_SLOTS:
  void onTypeChanged();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
