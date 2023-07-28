// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqTimerLogDisplay_h
#define pqTimerLogDisplay_h

#include "pqComponentsModule.h"
#include <QDialog>

class pqTimerLogDisplayUi;

class vtkPVTimerInformation;

class PQCOMPONENTS_EXPORT pqTimerLogDisplay : public QDialog
{
  Q_OBJECT

public:
  pqTimerLogDisplay(QWidget* p = nullptr);
  ~pqTimerLogDisplay() override;
  typedef QDialog Superclass;

  float timeThreshold() const;
  int bufferLength() const;
  bool isEnabled() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void refresh();
  void clear();
  void setTimeThreshold(float value);
  void setBufferLength(int value);
  void setEnable(bool state);
  void save();
  void save(const QString& filename);
  void save(const QStringList& files);

  void saveState();
  void restoreState();

protected:
  virtual void addToLog(const QString& source, vtkPVTimerInformation* timerInfo);

  void showEvent(QShowEvent*) override;
  void hideEvent(QHideEvent*) override;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setTimeThresholdById(int id);
  void setBufferLengthById(int id);

private:
  Q_DISABLE_COPY(pqTimerLogDisplay)

  double LogThreshold;
  pqTimerLogDisplayUi* ui;
};

#endif // pqTimerLogDisplay_h
