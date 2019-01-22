/*=========================================================================

  Program:   ParaView
  Module:    TestImageCompressors.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageCompressor.h"
#include "vtkImageData.h"
#include "vtkLZ4Compressor.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkSquirtCompressor.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkZlibImageCompressor.h"

#include <map>
#include <string>
#include <vtksys/CommandLineArguments.hxx>

#define TEST_SUCCESS 0
#define TEST_FAILED 1

class Data
{
public:
  double CompressTime;
  double DecompressTime;
  vtkIdType CompressedSize;
  Data()
    : CompressTime(0)
    , DecompressTime(0)
    , CompressedSize(0)
  {
  }
};
typedef std::map<std::string, Data> MapType;

bool DoTest(Data& data, vtkImageCompressor* compressor, vtkUnsignedCharArray* input)
{
  vtkNew<vtkUnsignedCharArray> outputCompressed;
  vtkNew<vtkUnsignedCharArray> outputDeCompressed;
  outputDeCompressed->SetNumberOfComponents(input->GetNumberOfComponents());
  outputDeCompressed->SetNumberOfTuples(input->GetNumberOfTuples());

  compressor->SetInput(input);
  compressor->SetOutput(outputCompressed.Get());

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  if (!compressor->Compress())
  {
    return false;
  }
  timer->StopTimer();
  data.CompressTime += timer->GetElapsedTime();

  compressor->SetInput(outputCompressed.Get());
  compressor->SetOutput(outputDeCompressed.Get());
  timer->StartTimer();
  if (!compressor->Decompress())
  {
    return false;
  }
  timer->StopTimer();
  data.DecompressTime += timer->GetElapsedTime();
  data.CompressedSize =
    outputCompressed->GetNumberOfTuples() * outputCompressed->GetNumberOfComponents();
  return true;
}

int TestImageCompressors(int argc, char* argv[])
{
  int max_count = 10;
  bool test_lossy = true;
  std::string imageFile;

  // Use --image argument to use this for benchmarking.
  vtksys::CommandLineArguments arg;
  arg.Initialize(argc, argv);
  typedef vtksys::CommandLineArguments argT;
  arg.AddArgument("--image", argT::EQUAL_ARGUMENT, &imageFile,
    "Optionally specify an image to use for compressing.");
  arg.StoreUnusedArguments(true);
  if (!arg.Parse())
  {
    cerr << "Problem parsing arguments" << endl;
    return TEST_FAILED;
  }

  vtkSmartPointer<vtkImageData> image;
  if (imageFile.empty())
  {
    vtkNew<vtkTesting> testing;
    testing->AddArguments(argc, (const char**)(argv));
    imageFile = testing->GetDataRoot();
    imageFile += "/Testing/Data/NE2_ps_bath.png";
    max_count = 1;
    test_lossy = false;
  }

  vtkNew<vtkPNGReader> reader;
  reader->SetFileName(imageFile.c_str());
  reader->Update();
  image = reader->GetOutput();

  vtkSmartPointer<vtkUnsignedCharArray> input =
    vtkUnsignedCharArray::SafeDownCast(image->GetPointData()->GetScalars());
  vtkIdType uncompressedSize = input->GetNumberOfTuples() * input->GetNumberOfComponents();

  MapType datas;
  for (int cc = 0; cc < max_count; cc++)
  {
    vtkNew<vtkLZ4Compressor> lz4;
    lz4->SetQuality(0);
    if (!DoTest(datas["LZ4 (quality: 0)"], lz4.Get(), input))
    {
      return TEST_FAILED;
    }
    if (test_lossy)
    {
      lz4->SetQuality(3);
      lz4->SetLossLessMode(0);
      if (!DoTest(datas["LZ4 (quality: 3)"], lz4.Get(), input))
      {
        return TEST_FAILED;
      }
      lz4->SetQuality(5);
      lz4->SetLossLessMode(0);
      if (!DoTest(datas["LZ4 (quality: 5)"], lz4.Get(), input))
      {
        return TEST_FAILED;
      }
    }

    vtkNew<vtkSquirtCompressor> squirt;
    squirt->SetSquirtLevel(0);
    if (!DoTest(datas["SQUIRT (squirt-level: 0)"], squirt.Get(), input))
    {
      return TEST_FAILED;
    }

    if (test_lossy)
    {
      squirt->SetSquirtLevel(3);
      if (!DoTest(datas["SQUIRT (squirt-level: 3)"], squirt.Get(), input))
      {
        return TEST_FAILED;
      }

      squirt->SetSquirtLevel(5);
      squirt->SetLossLessMode(0);
      if (!DoTest(datas["SQUIRT (squirt-level: 5)"], squirt.Get(), input))
      {
        return TEST_FAILED;
      }
    }

    vtkNew<vtkZlibImageCompressor> zlib;
    zlib->SetCompressionLevel(1);
    if (!DoTest(datas["ZLIB (compression-level: 1, color-space: 0)"], zlib.Get(), input))
    {
      return TEST_FAILED;
    }

    if (test_lossy)
    {
      zlib->SetCompressionLevel(1);
      zlib->SetColorSpace(3);
      zlib->SetLossLessMode(0);
      if (!DoTest(datas["ZLIB (compression-level: 1, color-space: 3)"], zlib.Get(), input))
      {
        return TEST_FAILED;
      }

      zlib->SetCompressionLevel(9);
      zlib->SetColorSpace(5);
      zlib->SetLossLessMode(0);
      if (!DoTest(datas["ZLIB (compression-level: 9, color-space: 5)"], zlib.Get(), input))
      {
        return TEST_FAILED;
      }
    }
  }

  cout << "Input: " << image->GetDimensions()[0] << "x" << image->GetDimensions()[1] << "x"
       << image->GetDimensions()[2] << " (uncompressed size: " << uncompressedSize << ") " << endl;

  for (MapType::iterator iter = datas.begin(); iter != datas.end(); ++iter)
  {
    cout << iter->first.c_str() << " :"
         << " compress: " << (iter->second.CompressTime / max_count)
         << " decompress: " << (iter->second.DecompressTime / max_count) << " compression ratio: "
         << ((uncompressedSize - iter->second.CompressedSize) * 100.0 / uncompressedSize)
         << "( compressed size: " << iter->second.CompressedSize << ")" << endl;
  }
  return TEST_SUCCESS;
}
