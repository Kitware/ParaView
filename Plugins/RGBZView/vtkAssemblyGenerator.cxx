/*=========================================================================

  Program:   ParaView
  Module:    vtkAssemblyGenerator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAssemblyGenerator.h"

#include "vtkDataSetReader.h"
#include "vtkDataSetWriter.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <set>
#include <sstream>
#include <string>
#include <vector>

//----------------------------------------------------------------------------
namespace {
  struct PixelOrder
  {
    float z;
    unsigned char idx;

    PixelOrder(unsigned char cIdx, float zBuffer)
    {
      this->idx = cIdx;
      this->z = zBuffer;
    }

    bool operator< (const PixelOrder& other) const
    {
      return this->z < other.z;
    }
  };

  void encode1(char* buffer, unsigned char* order, vtkIdType count)
  {
    const char* CODING_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/";
    vtkIdType offest = 0;
    buffer[0] = '+';
    buffer[1] = '\0';
    for(vtkIdType idx = 0; idx < count; ++idx)
      {
      if(order[idx] == 0)
        {
        --offest;
        }
      else
        {
        buffer[idx+offest] = CODING_TABLE[order[idx]-1];
        buffer[idx+offest+1] = '+';
        buffer[idx+offest+2] = '\0';
        }
      }
  }
};
//----------------------------------------------------------------------------
struct vtkAssemblyGenerator::vtkInternal
{
  std::vector<std::string> FileNames;
};
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAssemblyGenerator);

//----------------------------------------------------------------------------
vtkAssemblyGenerator::vtkAssemblyGenerator()
{
  this->DestinationDirectory = NULL;
  this->Internal = new vtkInternal;
}

//-----------------------------------------------------------------------------
vtkAssemblyGenerator::~vtkAssemblyGenerator()
{
  this->SetDestinationDirectory(NULL);
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkAssemblyGenerator::AddFileName(const char* name)
{
  this->Internal->FileNames.push_back(name);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAssemblyGenerator::RemoveAllFileNames()
{
  this->Internal->FileNames.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkAssemblyGenerator::GetNumberOfFileNames()
{
  return static_cast<unsigned int>(this->Internal->FileNames.size());
}

//----------------------------------------------------------------------------
const char* vtkAssemblyGenerator::GetFileName(unsigned int idx)
{
  if (idx >= this->Internal->FileNames.size())
    {
    return 0;
    }
  return this->Internal->FileNames[idx].c_str();
}

//-----------------------------------------------------------------------------
void vtkAssemblyGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkAssemblyGenerator::Write()
{
  if(this->DestinationDirectory == NULL)
    {
    return;
    }

  // Load all data
  std::vector<vtkSmartPointer<vtkImageData> > data;
  vtkIdType count = this->Internal->FileNames.size();
  for(vtkIdType i = 0; i < count; ++i)
    {
    vtkNew<vtkDataSetReader> reader;
    reader->SetFileName(this->GetFileName(i));
    reader->Update();
    data.push_back(vtkImageData::SafeDownCast(reader->GetOutput()));
    }

  // Write images
  vtkNew<vtkJPEGWriter> writer;
  for(unsigned int i = 0; i < count; ++i)
    {
    std::stringstream ss;
    ss << this->DestinationDirectory << "/" << i << ".jpg";
    writer->SetFileName(ss.str().c_str());
    writer->SetInputData(data[i]);
    writer->Write();
    }

  // Write composition info
  vtkIdType size = data[0]->GetNumberOfPoints();
  unsigned char compositeCount = static_cast<unsigned char>(count);

  vtkNew<vtkImageData> zOrder;
  zOrder->SetDimensions(data[0]->GetDimensions());
  zOrder->SetSpacing(1,1,1);
  zOrder->SetOrigin(0,0,0);
  vtkNew<vtkUnsignedCharArray> order;
  order->SetName("Ordering");
  order->SetNumberOfComponents(count);
  order->SetNumberOfTuples(size);
  zOrder->GetPointData()->SetScalars(order.GetPointer());

  // Extract raw zBuffer pointers for faster access
  float *zBuffers[255];
  for(unsigned int i = 0; i < count; ++i)
    {
    zBuffers[i] = vtkFloatArray::SafeDownCast(data[i]->GetPointData()->GetArray("z"))->GetPointer(0);
    }

  // Compute and fill ordering
  std::set<PixelOrder> orderFinder;
  std::set<PixelOrder>::iterator orderIter;
  vtkIdType offset = 0;
  for(vtkIdType idx = 0; idx < size; ++idx)
    {
    orderFinder.clear();
    for(unsigned char compositeIdx = 0; compositeIdx < compositeCount; ++compositeIdx)
      {
      orderFinder.insert(PixelOrder(compositeIdx, zBuffers[compositeIdx][idx]));
      }
    offset = idx * count;
    for(orderIter = orderFinder.begin(); orderIter != orderFinder.end(); ++orderIter)
      {
      order->SetValue(offset++, orderIter->idx);
      }
    }

   // Write order
  //  vtkNew<vtkDataSetWriter> orderWriter;
  //  std::stringstream fName;
  //  fName << this->DestinationDirectory << "/order.vtk";
  //  orderWriter->SetFileName(fName.str().c_str());
  //  orderWriter->SetInputData(zOrder.GetPointer());
  //  orderWriter->Write();

  // Write order as info.json
  std::stringstream jsonFileName;
  jsonFileName << this->DestinationDirectory << "/info.json";
  ofstream file(jsonFileName.str().c_str(), ios::out);
  if (file.fail())
    {
    vtkErrorMacro("RecursiveWrite: Could not open file " <<
                  jsonFileName.str().c_str());
    return;
    }
  file << "{"
       << "\n\"dimensions\": [" << zOrder->GetDimensions()[0] << ", " << zOrder->GetDimensions()[1] << ", " << zOrder->GetDimensions()[2] << "]"
       << ",\n\"composite-size\": " << count
       << ",\n\"pixel-order\": \"";

  // Fill with string encoded ordering
  char buffer[32];
  for(vtkIdType idx = 0; idx < size; ++idx)
    {
    encode1(buffer, order->GetPointer(idx*count), count);
    file << buffer;
    }

  // Close file
  file << "\"\n}" << endl;
  file.close();
  file.flush();
}
