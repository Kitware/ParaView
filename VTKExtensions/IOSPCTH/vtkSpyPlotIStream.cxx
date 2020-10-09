#include "vtkSpyPlotIStream.h"
#include "vtkByteSwap.h"

int vtkSpyPlotIStream::ReadString(char* str, size_t len)
{
  this->IStream->read(str, len);
  if (len != static_cast<size_t>(this->IStream->gcount()))
  {
    return 0;
  }
  return 1;
}
//-----------------------------------------------------------------------------
int vtkSpyPlotIStream::ReadString(unsigned char* str, size_t len)
{
  this->IStream->read(reinterpret_cast<char*>(str), len);
  if (len != static_cast<size_t>(this->IStream->gcount()))
  {
    return 0;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotIStream::ReadInt32s(int* val, int num)
{
  size_t len = 4 * num;
  this->IStream->read(reinterpret_cast<char*>(val), len);
  if (len != static_cast<size_t>(this->IStream->gcount()))
  {
    return 0;
  }
  vtkByteSwap::SwapBERange(val, num);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotIStream::ReadInt32sNoSwap(int* val, int num)
{
  size_t len = 4 * num;

  //
  // We are not going to check the length of the read, rely on the
  // read itself for errors.
  //
  // We are going to return the array unswapped. This is for performance.
  // Calling funcitons will have to deal with unswapped data.
  //
  if (!(this->IStream->read(reinterpret_cast<char*>(val), len)))
  {
    return 0;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotIStream::ReadInt64s(vtkTypeInt64* val, int num)
{
  int cc;
  for (cc = 0; cc < num; ++cc)
  {
    double d;
    int res = this->ReadDoubles(&d, 1);
    if (!res)
    {
      return 0;
    }
    *val = static_cast<vtkTypeInt64>(d);
    val++;
  }
  return 1;
}
//-----------------------------------------------------------------------------
int vtkSpyPlotIStream::ReadDoubles(double* val, int num)
{
  size_t len = 8 * num;
  this->IStream->read(reinterpret_cast<char*>(val), len);
  if (len != static_cast<size_t>(this->IStream->gcount()))
  {
    return 0;
  }
  vtkByteSwap::SwapBERange(val, num);
  return 1;
}

void vtkSpyPlotIStream::Seek(vtkTypeInt64 offset, bool rel)
{
  if (rel)
  {
    this->IStream->seekg(offset, ios::cur);
  }
  else
  {
    this->IStream->seekg(offset);
  }
}

vtkTypeInt64 vtkSpyPlotIStream::Tell()
{
  return this->IStream->tellg();
}

void vtkSpyPlotIStream::SetStream(istream* ist)
{
  if (!this->Buffer)
  {
    this->Buffer = new char[this->FileBufferSize];
  }

  std::streamsize s = sizeof(char) * (this->FileBufferSize - 1);
  ist->rdbuf()->pubsetbuf(this->Buffer, s);
  this->IStream = ist;
}

vtkSpyPlotIStream::vtkSpyPlotIStream()
  : FileBufferSize(2097152)
  , Buffer(0)
  , IStream(0)
{
}

vtkSpyPlotIStream::~vtkSpyPlotIStream()
{
  if (this->Buffer)
  {
    delete[] this->Buffer;
    this->Buffer = NULL;
  }
}
