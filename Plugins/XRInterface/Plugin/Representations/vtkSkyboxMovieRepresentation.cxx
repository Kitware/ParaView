// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSkyboxMovieRepresentation.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataSetAttributes.h"
#include "vtkFFMPEGVideoSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLMovieSphere.h"
#include "vtkPVRenderView.h"
#include "vtkPointSource.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkTable.h"

#include <QAudioOutput>

namespace
{
std::string vtkExtractString(vtkDataObject* data)
{
  std::string text;
  vtkFieldData* fieldData = data->GetFieldData();
  vtkAbstractArray* array = fieldData->GetAbstractArray(0);
  if (array && array->GetNumberOfTuples() > 0)
  {
    text = array->GetVariantValue(0).ToString();
  }
  return text;
}

class audioClass
{
public:
  QAudioOutput* AudioOutput = nullptr;
  QIODevice* IO = nullptr;
  const size_t TempBufferSize = 400000;
  char* TempBuffer[400000];
};

// mote that the audio callback is called from a spawned thread
// in the FFMPEGVideoSource so it is not the main thread. Blocking here
// does not block the main thread.
void AudioCallback(audioClass* self, vtkFFMPEGVideoSourceAudioCallbackData const& acbd)
{
  int destBytesPerSample = 2;
  if (acbd.DataType == VTK_FLOAT || acbd.DataType == VTK_DOUBLE)
  {
    destBytesPerSample = 4;
  }

  int destChannels = acbd.NumberOfChannels;
  bool sphericalAudio = false;
  if (acbd.NumberOfChannels == 4)
  {
    sphericalAudio = true;
    destChannels = 2;
  }

  bool isFloat = false;
  if (acbd.DataType == VTK_FLOAT || acbd.DataType == VTK_DOUBLE)
  {
    isFloat = true;
  }

  if (!self->AudioOutput)
  {
    QAudioFormat format;

    // Set up the format
    format.setSampleRate(acbd.SampleRate);
    format.setChannelCount(destChannels);
    format.setSampleSize(8 * destBytesPerSample);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(isFloat ? QAudioFormat::Float : QAudioFormat::UnSignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format))
    {
      vtkGenericWarningMacro("Raw audio format not supported by backend, cannot play audio.");
      return;
    }

    self->AudioOutput = new QAudioOutput(format);
    // if a platform has issue with the default buffer size
    // you can uncomment the line below to make it explicit
    // self->AudioOutput->setBufferSize(2 * 4 * 9600);
    self->IO = self->AudioOutput->start();
  }

  size_t bytesToWrite = destBytesPerSample * acbd.NumberOfSamples * destChannels;
  size_t bytesWritten = 0;
  while (bytesWritten < bytesToWrite)
  {
    size_t chunkSize = bytesToWrite - bytesWritten;
    auto outputBytesFree = self->AudioOutput->bytesFree();
    if (outputBytesFree < chunkSize)
    {
      chunkSize = outputBytesFree;
    }
    // round down to a sample boundary
    if (chunkSize % (destChannels * destBytesPerSample))
    {
      chunkSize =
        (destChannels * destBytesPerSample) * (chunkSize / (destChannels * destBytesPerSample));
    }
    if (chunkSize)
    {
      if (sphericalAudio)
      {
        if (chunkSize > self->TempBufferSize)
        {
          chunkSize = self->TempBufferSize;
        }
        // copy and process audio to new buffer
        float** inp = reinterpret_cast<float**>(acbd.Data);
        float* newVals = reinterpret_cast<float*>(self->TempBuffer);
        float* fdest = newVals;
        float inValues[4];
        size_t samplesWritten = bytesWritten / (destBytesPerSample * destChannels);
        size_t chunkSamples = chunkSize / (destBytesPerSample * destChannels);
        for (size_t i = samplesWritten; i < samplesWritten + chunkSamples; ++i)
        {
          // get input values
          if (acbd.Packed)
          {
            float* inl = inp[0] + i * 4;
            inValues[0] = *inl++;
            inValues[1] = *inl++;
            inValues[2] = *inl++;
            inValues[3] = *inl++;
          }
          else
          {
            inValues[0] = *(inp[0] + i);
            inValues[1] = *(inp[1] + i);
            inValues[2] = *(inp[2] + i);
            inValues[3] = *(inp[3] + i);
          }

          // write output values
          // the normalization factors depend on n3d versus sn3d versus maxN etc
          // I have no clue what should be used here but see
          // https://en.wikipedia.org/wiki/Ambisonic_data_exchange_formats
          // for more background
          *fdest++ = inValues[0] + inValues[3];
          *fdest++ = inValues[0] - inValues[3];
        }
        self->IO->write(reinterpret_cast<char*>(newVals), chunkSize);
      }
      else
      {
        // if it is packed then we can just pass it along
        if (acbd.Packed)
        {
          char* dptr = reinterpret_cast<char*>(acbd.Data[0] + bytesWritten);
          self->IO->write(dptr, chunkSize);
        }
        else // otherwise we have to unpack
        {
          if (chunkSize > self->TempBufferSize)
          {
            chunkSize = self->TempBufferSize;
          }
          // copy and process audio to new buffer
          float** inp = reinterpret_cast<float**>(acbd.Data);
          float* newVals = reinterpret_cast<float*>(self->TempBuffer);
          float* fdest = newVals;
          size_t samplesWritten = bytesWritten / (destBytesPerSample * destChannels);
          size_t chunkSamples = chunkSize / (destBytesPerSample * destChannels);
          for (size_t i = samplesWritten; i < samplesWritten + chunkSamples; ++i)
          {
            *fdest++ = *(inp[0] + i);
            *fdest++ = *(inp[1] + i);
          }
          self->IO->write(reinterpret_cast<char*>(newVals), chunkSize);
        }
      }
      bytesWritten += chunkSize;
    }
  }
}

} // end anonymous namespace

vtkStandardNewMacro(vtkSkyboxMovieRepresentation);

//----------------------------------------------------------------------------
vtkSkyboxMovieRepresentation::vtkSkyboxMovieRepresentation()
{
  this->InternalAudioClass = new audioClass();
  vtkPointSource* source = vtkPointSource::New();
  source->SetNumberOfPoints(1);
  source->Update();
  this->DummyPolyData->ShallowCopy(source->GetOutputDataObject(0));
  source->Delete();

  this->Actor->SetProjectionToSphere();
}

//----------------------------------------------------------------------------
vtkSkyboxMovieRepresentation::~vtkSkyboxMovieRepresentation()
{
  delete this->InternalAudioClass;

  if (this->MovieSource)
  {
    this->MovieSource->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkSkyboxMovieRepresentation::SetVisibility(bool val)
{
  // play when visibility is turned on
  if (val && this->Play)
  {
    // update the filename if set in GUI
    this->Update();
    this->SetPlay(true);
  }

  // stop when visibility is turned off
  if (!val && this->Play && this->MovieSource)
  {
    this->MovieSource->Stop();
  }

  this->Actor->SetVisibility(val ? 1 : 0);
  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
int vtkSkyboxMovieRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
bool vtkSkyboxMovieRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);
    return this->Superclass::AddToView(rview);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSkyboxMovieRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return this->Superclass::RemoveFromView(rview);
  }
  return false;
}

//----------------------------------------------------------------------------
int vtkSkyboxMovieRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkTable* input = vtkTable::GetData(inputVector[0], 0);
    if (input->GetNumberOfRows() > 0 && input->GetNumberOfColumns() > 0)
    {
      this->DummyPolyData->GetFieldData()->ShallowCopy(input->GetRowData());

      std::string text = vtkExtractString(this->DummyPolyData);
      this->FileName = text;
    }
  }
  else
  {
    this->DummyPolyData->Initialize();
  }
  this->DummyPolyData->Modified();
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkSkyboxMovieRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    // i.e. this->GetVisibility() == false, hence nothing to do.
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetPiece(inInfo, this, this->DummyPolyData);
    vtkPVRenderView::SetDeliverToClientAndRenderingProcesses(inInfo, this,
      /*deliver_to_client=*/true, /*gather_before_delivery=*/false);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    // since there's no direct connection between the mapper and the collector,
    // we don't put an update-suppressor in the pipeline.
  }

  return 1;
}

void vtkSkyboxMovieRepresentation::SetProjection(int val)
{
  this->Actor->SetProjection(val);
}

void vtkSkyboxMovieRepresentation::SetFloorPlane(float a, float b, float c, float d)
{
  this->Actor->SetFloorPlane(a, b, c, d);
}

void vtkSkyboxMovieRepresentation::SetFloorRight(float a, float b, float c)
{
  this->Actor->SetFloorRight(a, b, c);
}

void vtkSkyboxMovieRepresentation::SetPlay(bool val)
{
  if (!this->FileName.length())
  {
    return;
  }

  if (val)
  {
    if (this->MovieSource)
    {
      this->MovieSource->Stop();
      this->MovieSource->Delete();
    }
    this->MovieSource = vtkFFMPEGVideoSource::New();
    this->MovieSource->SetFileName(this->FileName.c_str());
    this->Actor->SetVideoSource(this->MovieSource);

    // setup audio playback
    this->MovieSource->SetAudioCallback(
      std::bind(
        &AudioCallback, static_cast<audioClass*>(this->InternalAudioClass), std::placeholders::_1),
      nullptr);

    this->MovieSource->Record();
  }
  else if (this->MovieSource)
  {
    this->MovieSource->Stop();
  }
  this->Play = val;
}

//----------------------------------------------------------------------------
void vtkSkyboxMovieRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
