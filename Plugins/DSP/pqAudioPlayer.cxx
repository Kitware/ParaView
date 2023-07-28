// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqAudioPlayer.h"
#include "ui_pqAudioPlayer.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqCoreUtilities.h>
#include <pqPipelineSource.h>
#include <pqServer.h>

#include "vtkSMPTools.h"
#include <vtkArrayDispatch.h>
#include <vtkAssume.h>
#include <vtkDataArrayRange.h>
#include <vtkDataObject.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkPVArrayInformation.h>
#include <vtkPVDataInformation.h>
#include <vtkPVDataMover.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkShortArray.h>
#include <vtkTable.h>
#include <vtkWeakPointer.h>

#include <vtksys/SystemTools.hxx>

#include <QApplication>
#include <QAudioOutput>
#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QStyle>
#include <QtMath>

#include <cmath>
#include <string>

namespace
{
const int DEFAULT_SAMPLE_RATE = 44100;

template <typename ArrayType, typename ValueType>
void fillByteArray(ArrayType* signal, unsigned int sampleSize, QByteArray* byteArray)
{
  // Fill byte array
  vtkIdType numberOfSamples = signal->GetNumberOfValues();
  byteArray->resize(sampleSize * numberOfSamples);

  vtkSMPTools::For(0, numberOfSamples, [&](unsigned int begin, unsigned int end) {
    const auto range = vtk::DataArrayValueRange<1>(signal, begin, end);
    unsigned int byteIdx = begin * sampleSize;
    for (const auto sample : range)
    {
      ValueType s = static_cast<ValueType>(sample);
      char* bytePtr = reinterpret_cast<char*>(&s);
      for (unsigned int i = 0; i < sampleSize; i++)
      {
        (*byteArray)[byteIdx++] = *(bytePtr + i);
      }
    }
  });
}

struct SetupAndFill
{
  using SIntegers = vtkTypeList::Create<char, short, int, long, long long, signed char>;
  using UIntegers = vtkTypeList::Create<unsigned char, unsigned short, unsigned int, unsigned long,
    unsigned long long>;

  // Signed integers
  template <typename ArrayType, typename ValueType = vtk::GetAPIType<ArrayType>,
    typename std::enable_if<vtkTypeList::IndexOf<SIntegers, ValueType>::Result != -1>::type* =
      nullptr>
  void operator()(ArrayType* signal, unsigned int& sampleSize, QAudioFormat::SampleType& sampleType,
    QByteArray* byteArray)
  {
    sampleSize = sizeof(ValueType);
    sampleType = QAudioFormat::SignedInt;
    fillByteArray<ArrayType, ValueType>(signal, sampleSize, byteArray);
  }

  // Unsigned integers
  template <typename ArrayType, typename ValueType = vtk::GetAPIType<ArrayType>,
    typename std::enable_if<vtkTypeList::IndexOf<UIntegers, ValueType>::Result != -1>::type* =
      nullptr>
  void operator()(ArrayType* signal, unsigned int& sampleSize, QAudioFormat::SampleType& sampleType,
    QByteArray* byteArray)
  {
    sampleSize = sizeof(ValueType);
    sampleType = QAudioFormat::UnSignedInt;
    fillByteArray<ArrayType, ValueType>(signal, sampleSize, byteArray);
  }

  // Floating types : cast double to float
  template <typename ArrayType, typename ValueType = vtk::GetAPIType<ArrayType>,
    typename std::enable_if<vtkTypeList::IndexOf<vtkArrayDispatch::Reals, ValueType>::Result !=
      -1>::type* = nullptr>
  void operator()(ArrayType* signal, unsigned int& sampleSize, QAudioFormat::SampleType& sampleType,
    QByteArray* byteArray)
  {
    sampleSize = sizeof(float);
    sampleType = QAudioFormat::Float;
    fillByteArray<ArrayType, float>(signal, sampleSize, byteArray);
  }
};
}

//=============================================================================
class pqAudioPlayer::pqInternals : public Ui::pqAudioPlayer
{
public:
  pqInternals();
  ~pqInternals();

  /**
   * @brief Play the current audio buffer.
   */
  void play();

  /**
   * @brief Suspend the current audio stream.
   */
  void pause();

  /**
   * @brief Go to the beginning of the audio buffer.
   */
  void rewind();

  /**
   * @brief Set the volume to the desired value,
   * expressed as a percentage of the original signal volume.
   * The value will be clamped between 0 and 100.
   */
  void setVolume(int value);

  /**
   * @brief Fetch data and prepare the internal structures.
   * @return True if the setup was succesfully done.
   *
   * If the output of the active source is a vtkTable,
   * fetch the data from it (selected column containing audio samples)
   * and prepare internal buffers and audio objects with it.
   */
  bool fetchAndPrepareData();

  /**
   * @brief Update the UI with the currently available meta-data.
   * @return True if data is available.
   *
   * Update the name of the active source displayed in the dock widget and
   * fill the combo-box with the available data arrays.
   */
  bool fetchMetaData();

  /**
   * @brief Enable/Disable the play/pause button.
   */
  void enablePlayerPanel(bool enable = true);

  /**
   * @brief Convenience method to swap from pause to play button.
   */
  void swapToPlayButton(bool play = true);

  vtkWeakPointer<vtkSMSourceProxy> ActiveSourceProxy;
  std::string ArrayName;
  QSharedPointer<QBuffer> AudioBuffer;
  QSharedPointer<QAudioOutput> AudioOutput;
  QScopedPointer<QByteArray> ByteArray;
  int DefaultSampleRate = DEFAULT_SAMPLE_RATE;
  bool NeedUpdate = false;
};

//-----------------------------------------------------------------------------
pqAudioPlayer::pqInternals::pqInternals()
  : ByteArray(new QByteArray())
{
  pqCoreUtilities::promptUser("pqAudioPlayer", QMessageBox::Information,
    "Audio Player Volume Information",
    "You are currently using the Audio Player Dock Widget.\n\n"
    "To prevent any damage to your audition, please leave the original volume "
    "value of the audio player unchanged (5%) when trying to read some data "
    "for the first time, and adjust it afterwards if needed.",
    QMessageBox::Ok | QMessageBox::Save);
}

//-----------------------------------------------------------------------------
pqAudioPlayer::pqInternals::~pqInternals()
{
  if (this->AudioOutput)
  {
    this->AudioOutput->stop();
  }
  this->AudioOutput.clear();

  if (this->AudioBuffer)
  {
    this->AudioBuffer->close();
  }
  this->AudioBuffer.clear();
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::pqInternals::play()
{
  if (!this->AudioOutput)
  {
    return;
  }
  this->AudioOutput->start(this->AudioBuffer.get());
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::pqInternals::pause()
{
  if (!this->AudioOutput)
  {
    return;
  }
  this->AudioOutput->suspend();
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::pqInternals::rewind()
{
  if (!this->AudioOutput)
  {
    return;
  }

  // Pause and go to the beginning of the audio buffer
  this->AudioOutput->suspend();
  this->AudioBuffer->seek(0);
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::pqInternals::setVolume(int value)
{
  this->VolumeValueLabel->setText(QString::number(value) + " %");

  if (!this->AudioOutput)
  {
    return;
  }

  // Convert from logarithmic to linear scale
  qreal linearVolume = QAudio::convertVolume(
    value / qreal(100.0), QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale);

  this->AudioOutput->setVolume(linearVolume);
}

//-----------------------------------------------------------------------------
bool pqAudioPlayer::pqInternals::fetchAndPrepareData()
{
  // Stop player, clean current audio output and audio buffer (if any)
  if (this->AudioOutput)
  {
    this->AudioOutput->stop();
    this->AudioOutput.clear();
    this->AudioBuffer->close();
    this->AudioBuffer.clear();
  }

  // If there is no current active source, no need to continue
  if (!this->ActiveSourceProxy)
  {
    return false;
  }

  // Fetch the data from the output of the active source
  pqServer* server = pqActiveObjects::instance().activeServer();
  vtkSMSessionProxyManager* pxm = server->proxyManager();

  vtkSmartPointer<vtkSMProxy> dataMoverProxy =
    vtk::TakeSmartPointer(pxm->NewProxy("misc", "DataMover"));
  vtkSMPropertyHelper(dataMoverProxy, "Producer").Set(this->ActiveSourceProxy);
  dataMoverProxy->UpdateVTKObjects();
  dataMoverProxy->InvokeCommand("Execute");
  vtkPVDataMover* dataMover = vtkPVDataMover::SafeDownCast(dataMoverProxy->GetClientSideObject());

  if (dataMover->GetNumberOfDataSets() == 0)
  {
    qWarning("No data to read sound from.");
    return false;
  }

  vtkDataObject* activeObject = dataMover->GetDataSetAtIndex(0);
  vtkTable* activeTable = vtkTable::SafeDownCast(activeObject);
  if (!activeTable)
  {
    qWarning("No table to read sound from.");
    return false;
  }

  // Retrieve the audio signal from the vtkTable
  std::string arrayName = this->DataSelectionComboBox->currentText().toStdString();
  vtkAbstractArray* column = activeTable->GetColumnByName(arrayName.c_str());

  vtkDataArray* signal = vtkDataArray::SafeDownCast(column);
  if (!signal)
  {
    qWarning("Unable to retrieve the signal from the table.");
    return false;
  }
  if (signal->GetNumberOfValues() <= 0)
  {
    qWarning("The signal is empty (no samples to read).");
    return false;
  }

  // Find the right audio format parameters and fill the byte buffer.
  // This step depends on the signal value type so we fall back on an array dispatcher.
  using SupportedTypes = vtkTypeList::Create<char, unsigned char, short, unsigned short, int,
    unsigned int, long, unsigned long, long long, unsigned long long, float, double>;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<SupportedTypes>;

  SetupAndFill worker;
  unsigned int sampleSize = 0; // In bytes
  QAudioFormat::SampleType sampleType = QAudioFormat::Unknown;

  if (!Dispatcher::Execute(signal, worker, sampleSize, sampleType, this->ByteArray.get()))
  {
    qWarning("Unable to process input signal.");
    return false;
  }

  // Setup audio format
  QAudioFormat audioFormat;
  audioFormat.setSampleRate(this->SampleRateSpinBox->value());
  audioFormat.setChannelCount(1);
  audioFormat.setSampleSize(sampleSize * 8); // In bits
  audioFormat.setCodec("audio/pcm");
  audioFormat.setByteOrder(QAudioFormat::LittleEndian);
  audioFormat.setSampleType(sampleType);

  // Look for a device that supports the format
  auto isDeviceValid = [audioFormat](const QAudioDeviceInfo& info) {
    return !info.isNull() && info.isFormatSupported(audioFormat) &&
      !info.supportedCodecs().empty() && !info.supportedSampleRates().empty() &&
      !info.supportedSampleTypes().empty() && !info.supportedSampleSizes().empty() &&
      !info.supportedChannelCounts().empty() && !info.supportedByteOrders().empty();
  };

  QAudioDeviceInfo foundDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
  if (!isDeviceValid(foundDeviceInfo))
  {
    const auto deviceInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    bool foundDeviceValid = false;
    for (const QAudioDeviceInfo& deviceInfo : deviceInfos)
    {
      if (isDeviceValid(deviceInfo))
      {
        foundDeviceValid = true;
        foundDeviceInfo = deviceInfo;
        break;
      }
    }

    // If we still couldn't find a device that supports the output format then abort
    if (!foundDeviceValid)
    {
      qWarning() << "This audio format is not supported by any of your audio devices "
                    "(sample rate = "
                 << audioFormat.sampleRate()
                 << " Hz, "
                    "sample size = "
                 << audioFormat.sampleSize()
                 << " bits, "
                    "sample type = "
                 << audioFormat.sampleType() << ").";
      return false;
    }
  }

  // Setup final audio buffer
  this->AudioBuffer = QSharedPointer<QBuffer>(new QBuffer(this->ByteArray.get()));
  this->AudioBuffer->open(QIODevice::ReadOnly);

  // Setup audio output
  this->AudioOutput = QSharedPointer<QAudioOutput>(new QAudioOutput(foundDeviceInfo, audioFormat));
  this->setVolume(this->VolumeSlider->value());

  return true;
}

//-----------------------------------------------------------------------------
bool pqAudioPlayer::pqInternals::fetchMetaData()
{
  // Default sample rate if no meta-data about it is found
  this->DefaultSampleRate = DEFAULT_SAMPLE_RATE;

  // Retrieve the active source proxy
  if (!this->ActiveSourceProxy)
  {
    // Add bold style
    this->SourcePanel->setText("<span style=\" font-weight:600;\">None</span>\n");
    this->DataSelectionComboBox->clear();
    this->SampleRateSpinBox->setValue(this->DefaultSampleRate);
    return false;
  }

  // Retrieve and update in the UI the name of the active source
  QString sourceName(this->ActiveSourceProxy->GetLogName());
  // Add bold style
  this->SourcePanel->setText("<span style=\" font-weight:600;\">" + sourceName + "</span>\n");

  // Retrieve meta-data from the active source
  vtkPVDataInformation* sourceInfo = this->ActiveSourceProxy->GetDataInformation();
  if (!sourceInfo)
  {
    qWarning("Failed to retrieve information from the source proxy.");
    return false;
  }

  // Search for "sample_rate" field data to extract sample rate from
  vtkPVDataSetAttributesInformation* fieldInfo =
    sourceInfo->GetAttributeInformation(vtkDataObject::FIELD);
  if (!fieldInfo)
  {
    qWarning("Failed to retrieve field information from the source proxy.");
    return false;
  }

  vtkPVArrayInformation* sampleRateArrayInfo = fieldInfo->GetArrayInformation("sample_rate");

  if (sampleRateArrayInfo)
  {
    if (sampleRateArrayInfo->GetNumberOfComponents() != 1 ||
      sampleRateArrayInfo->GetNumberOfTuples() != 1)
    {
      qInfo() << "Sample rate data is ill-formed - skipping";
    }
    else
    {
      double value = sampleRateArrayInfo->GetComponentRange(0)[0];
      if (value <= 0)
      {
        qInfo() << "Sample rate must be a stricly positive value - skipping";
      }
      this->DefaultSampleRate = static_cast<int>(value);
    }
  }

  // Retrieve the columns (audio signals) from the table
  vtkPVDataSetAttributesInformation* rowInfo =
    sourceInfo->GetAttributeInformation(vtkDataObject::ROW);
  if (!rowInfo)
  {
    qWarning("Failed to retrieve row information from the source proxy.");
    return false;
  }

  // Update the data shown in the combo-box
  // Also search for "time" row data
  this->DataSelectionComboBox->clear();
  for (int i = 0; i < rowInfo->GetNumberOfArrays(); i++)
  {
    const char* arrayName = rowInfo->GetArrayInformation(i)->GetName();

    // If "time" row data is present, extract sample rate from it
    if (vtksys::SystemTools::Strucmp(arrayName, "time") == 0)
    {
      vtkPVArrayInformation* timeArrayInfo = rowInfo->GetArrayInformation(i);
      if (timeArrayInfo->GetNumberOfComponents() != 1)
      {
        qInfo() << "Time data is ill-formed (number of components != 1) - skipping";
        continue;
      }

      const int numberOfTimesteps = timeArrayInfo->GetNumberOfTuples();
      const double* range = timeArrayInfo->GetComponentRange(0);
      if (range[1] == range[0])
      {
        qInfo() << "Time data range should be strictly positive - skipping";
        continue;
      }

      // Compute sample rate from time and enable corresponding option
      this->DefaultSampleRate = numberOfTimesteps / (range[1] - range[0]);
      continue;
    }
    this->DataSelectionComboBox->addItem(arrayName);
  }

  // Update the sample rate displayed in the UI
  this->SampleRateSpinBox->setValue(this->DefaultSampleRate);

  return true;
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::pqInternals::enablePlayerPanel(bool enable)
{
  this->PlayButton->setEnabled(enable);
  this->PauseButton->setEnabled(enable);
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::pqInternals::swapToPlayButton(bool play)
{
  this->PauseButton->setVisible(!play);
  this->PlayButton->setVisible(play);
}

//-----------------------------------------------------------------------------
pqAudioPlayer::pqAudioPlayer(const QString& title, QWidget* parent)
  : Superclass(title, parent)
  , Internals(new pqAudioPlayer::pqInternals())
{
  this->constructor();
}

//-----------------------------------------------------------------------------
pqAudioPlayer::pqAudioPlayer(QWidget* parent)
  : Superclass(parent)
  , Internals(new pqAudioPlayer::pqInternals())
{
  this->constructor();
}

//-----------------------------------------------------------------------------
pqAudioPlayer::~pqAudioPlayer() = default;

//-----------------------------------------------------------------------------
void pqAudioPlayer::constructor()
{
  this->setWindowTitle("Audio Player");
  QWidget* widget = new QWidget(this);
  this->Internals->setupUi(widget);
  this->setWidget(widget);

  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::sourceChanged, this,
    &pqAudioPlayer::onActiveSourceChanged);
  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::dataUpdated, this,
    &pqAudioPlayer::onPipelineUpdated);

  QObject::connect(
    this->Internals->PlayButton, &QPushButton::clicked, this, &pqAudioPlayer::onPlayButtonClicked);
  QObject::connect(this->Internals->PauseButton, &QPushButton::clicked, this,
    &pqAudioPlayer::onPauseButtonClicked);
  QObject::connect(
    this->Internals->StopButton, &QPushButton::clicked, this, &pqAudioPlayer::onStopButtonClicked);
  QObject::connect(this->Internals->ResetButton, &QPushButton::clicked, this,
    &pqAudioPlayer::onResetButtonClicked);
  QObject::connect(this->Internals->DataSelectionComboBox, &QComboBox::currentTextChanged, this,
    &pqAudioPlayer::onParametersChanged);
  QObject::connect(this->Internals->SampleRateSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
    this, &pqAudioPlayer::onParametersChanged);
  QObject::connect(
    this->Internals->VolumeSlider, &QSlider::valueChanged, this, &pqAudioPlayer::onVolumeChanged);

  this->Internals->enablePlayerPanel(false);
  this->Internals->swapToPlayButton();
  this->Internals->StopButton->setEnabled(false);

  // Make sure it is working properly when launching the plugin after we created a source
  if (auto* source = pqActiveObjects::instance().activeSource())
  {
    this->onActiveSourceChanged(source);
  }
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::onPlayButtonClicked()
{
  // First, check if the data need to be updated
  if (this->Internals->NeedUpdate)
  {
    // Fetch data and setup internal structures
    if (!this->Internals->fetchAndPrepareData())
    {
      qWarning() << "Unable to retrieve the audio signal.";
      return;
    }
  }

  // For monitoring the AudioOutput internal state
  QObject::connect(this->Internals->AudioOutput.get(), &QAudioOutput::stateChanged, this,
    &pqAudioPlayer::onPlayerStateChanged);

  // Update UI
  this->Internals->swapToPlayButton(false);
  if (!this->Internals->StopButton->isEnabled())
  {
    this->Internals->StopButton->setEnabled(true);
  }

  // Play
  this->Internals->play();
  this->Internals->NeedUpdate = false;
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::onPauseButtonClicked()
{
  // Update UI
  this->Internals->swapToPlayButton();
  this->Internals->PlayButton->show();

  // Pause
  this->Internals->pause();
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::onStopButtonClicked()
{
  // Update UI
  this->Internals->swapToPlayButton();
  this->Internals->StopButton->setEnabled(false);

  // Rewind
  this->Internals->rewind();
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::onPlayerStateChanged(QAudio::State newState)
{
  switch (newState)
  {
    case QAudio::IdleState:
      // Finished playing (no more data)
      this->onStopButtonClicked();
      break;
    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::onActiveSourceChanged(pqPipelineSource* activeSource)
{
  // Check if we have an active source
  if (!activeSource)
  {
    this->Internals->ActiveSourceProxy = nullptr;
  }
  else
  {
    // Store the active source proxy
    this->Internals->ActiveSourceProxy = vtkSMSourceProxy::SafeDownCast(activeSource->getProxy());
    if (!this->Internals->ActiveSourceProxy)
    {
      qCritical("Unable to retrieve the active source proxy from the active source.");
    }
  }

  // Update meta-data
  this->onPipelineUpdated();
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::onPipelineUpdated()
{
  // Update available data
  if (!this->Internals->fetchMetaData())
  {
    this->Internals->enablePlayerPanel(false);
    return;
  }
  this->Internals->enablePlayerPanel();
  this->Internals->VolumeSlider->setValue(5);
  this->Internals->NeedUpdate = true;
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::onResetButtonClicked()
{
  this->Internals->SampleRateSpinBox->setValue(this->Internals->DefaultSampleRate);
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::onParametersChanged()
{
  this->Internals->NeedUpdate = true;
}

//-----------------------------------------------------------------------------
void pqAudioPlayer::onVolumeChanged(int value)
{
  this->Internals->setVolume(value);
}
