// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqAudioPlayer_h
#define pqAudioPlayer_h

#include <QAudio>
#include <QDockWidget>

class pqPipelineSource;

/**
 * @brief The pqAudioPlayer dock widget allows to read audio data from the current active source.
 *
 * The pqAudioPlayer dock widget allows to read audio retrieved from the current active source.
 * The source has to produce a vtkTable, containing the audio signal. Each column of the vtkTable
 * is interpreted as a different audio signal and can be selected in the widget. Each row represent
 * a sample. The sample rate is selected manually in the widget. The default value can be
 * automatically configured by adding a field data array in the vtkTable named "sample_rate",
 * containing the value.
 *
 * Limitation : for now, the player only handles audio signals composed of shorts (vtkShortArray
 * attributes).
 */

class pqAudioPlayer : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  pqAudioPlayer(QWidget* parent = nullptr);
  pqAudioPlayer(const QString& title, QWidget* parent = nullptr);

  ~pqAudioPlayer() override;

protected Q_SLOTS:
  ///@{
  /**
   * Slots handling UI signals
   */
  void onPlayButtonClicked();
  void onPauseButtonClicked();
  void onStopButtonClicked();
  void onResetButtonClicked();
  void onParametersChanged();
  void onVolumeChanged(int value);
  ///@}

  ///@{
  /**
   * Slots handling application state changes
   */
  void onPlayerStateChanged(QAudio::State newState);
  void onActiveSourceChanged(pqPipelineSource* activeSource);
  void onPipelineUpdated();
  ///@}

private:
  Q_DISABLE_COPY(pqAudioPlayer)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  void constructor();
};

#endif // pqAudioPlayer_h
