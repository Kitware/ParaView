#include <iostream>     // std::cout
#include <algorithm>    // std::sort
#include <vector>       // std::vector

#include "vtkTimerLog.h"

namespace CompositeSort {

struct zSorter {
  zSorter(size_t image_width, size_t image_height, size_t buffer_size, float** zBuffer)
  {
    this->image_width = image_width;
    this->image_height = image_height;
    this->buffer_size = buffer_size;
    this->zBuffer = zBuffer;
    this->pixCount = image_width * image_height;
  }

  void process()
  {
    double tStart = vtkTimerLog::GetUniversalTime();

    // Init vector
    std::vector<size_t> pixelArray(this->pixCount * this->buffer_size);
    size_t pixIdx = this->pixCount;
    size_t idx, value, remaining;
    size_t delta_start = (this->pixCount * this->buffer_size);
    while(pixIdx--)
      {
      remaining = this->buffer_size;
      idx = (pixIdx * remaining);
      value = pixIdx + delta_start;
      while(remaining--)
        {
        value -= this->pixCount;
        pixelArray[idx++] = value;
        }
      }
    std::cout << "Fill memory time " << (vtkTimerLog::GetUniversalTime() - tStart) << std::endl;

    // Sort pixels
    tStart = vtkTimerLog::GetUniversalTime();
    std::sort(pixelArray.begin(), pixelArray.end(), *this);
    std::cout << "Sorting time " << (vtkTimerLog::GetUniversalTime() - tStart) << std::endl;
  }

  bool operator() (size_t a, size_t b)
  {
    size_t a_pix, b_pix;
    if((a_pix = a % this->pixCount) < (b_pix = b % this->pixCount))
      {
      return true;
      }
    else if(a_pix == b_pix)
      {
      size_t a_buf = a / this->pixCount;
      size_t b_buf = b / this->pixCount;
      float a_z, b_z;

      if((a_z = this->zBuffer[a_buf][a_pix]) < (b_z = this->zBuffer[b_buf][b_pix]))
        {
        return true;
        }
      else if(a_z == b_z)
        {
        return a_buf < b_buf;
        }
      }
    return false;
  }

  size_t image_width;
  size_t image_height;
  size_t buffer_size;
  size_t pixCount;
  float** zBuffer;
};

};

int main () {
  size_t w = 1000, h = 1000, size = 10;
  float **zBuffer = new float*[size];
  for(int i = 0; i < size; ++i)
    {
    zBuffer[i] = new float[w*h];
    }

  std::cout << "Init sorter for w: " << w << " h: " << h << " nbPix: " << (w*h) << " size: " << size << std::endl;
  CompositeSort::zSorter sorter(w, h, size, zBuffer);
  std::cout << "Start sort" << std::endl;
  sorter.process();
  std::cout << "Sort done" << std::endl;


  // Free memory
  for(int i = 0; i < size; ++i)
    {
    delete[] zBuffer[i];
    zBuffer[i] = NULL;
    }
  delete[] zBuffer;

  return 0;
}
