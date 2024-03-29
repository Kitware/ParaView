// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqImageUtil.h"

#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkSMUtilities.h"

#include <QBuffer>
#include <QImage>

// NOTES:
// - QImage pixel data is 32 bit aligned, whereas vtkImageData's pixel data
//   is not necessarily 32 bit aligned
// - QImage is a mirror of vtkImageData, which is taken care of by scanning
//   a row at a time and copying opposing rows

//-----------------------------------------------------------------------------
bool pqImageUtil::toImageData(const QImage& img, vtkImageData* vtkimage)
{
  int height = img.height();
  int width = img.width();
  int numcomponents = img.hasAlphaChannel() ? 4 : 3;

  vtkimage->SetExtent(0, width - 1, 0, height - 1, 0, 0);
  vtkimage->SetSpacing(1.0, 1.0, 1.0);
  vtkimage->SetOrigin(0.0, 0.0, 0.0);
  vtkimage->AllocateScalars(VTK_UNSIGNED_CHAR, numcomponents);
  for (int i = 0; i < height; i++)
  {
    unsigned char* row;
    row = static_cast<unsigned char*>(vtkimage->GetScalarPointer(0, height - i - 1, 0));
    const QRgb* linePixels = reinterpret_cast<const QRgb*>(img.scanLine(i));
    for (int j = 0; j < width; j++)
    {
      const QRgb& col = linePixels[j];
      row[j * numcomponents] = qRed(col);
      row[j * numcomponents + 1] = qGreen(col);
      row[j * numcomponents + 2] = qBlue(col);
      if (numcomponents == 4)
      {
        row[j * numcomponents + 3] = qAlpha(col);
      }
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool pqImageUtil::fromImageData(vtkImageData* vtkimage, QImage& img)
{
  if (vtkimage->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    return false;
  }

  int extent[6];
  vtkimage->GetExtent(extent);
  int width = extent[1] - extent[0] + 1;
  int height = extent[3] - extent[2] + 1;
  int numcomponents = vtkimage->GetNumberOfScalarComponents();
  if (!(numcomponents == 3 || numcomponents == 4))
  {
    return false;
  }

  QImage newimg(width, height, QImage::Format_ARGB32);

  for (int i = 0; i < height; i++)
  {
    QRgb* bits = reinterpret_cast<QRgb*>(newimg.scanLine(i));
    unsigned char* row;
    row = static_cast<unsigned char*>(
      vtkimage->GetScalarPointer(extent[0], extent[2] + height - i - 1, extent[4]));
    for (int j = 0; j < width; j++)
    {
      unsigned char* data = &row[j * numcomponents];
      bits[j] = numcomponents == 4 ? qRgba(data[0], data[1], data[2], data[3])
                                   : qRgb(data[0], data[1], data[2]);
    }
  }

  img = newimg;
  return true;
}

//-----------------------------------------------------------------------------
bool pqImageUtil::imageDataToFormatedByteArray(
  vtkImageData* vtkimage, QByteArray& bArray, const char* format)
{
  if (vtkimage->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    return false;
  }

  QImage qimg;
  if (!pqImageUtil::fromImageData(vtkimage, qimg))
  {
    return false;
  }

  QBuffer buff(&bArray);
  qimg.save(&buff, format);

  return true;
}

//-----------------------------------------------------------------------------
int pqImageUtil::saveImage(vtkImageData* vtkimage, const QString& filename, int quality /*=-1*/)
{
  int error_code = vtkErrorCode::NoError;
  if (!vtkimage)
  {
    return vtkErrorCode::UnknownError;
  }
  if (filename.isEmpty())
  {
    return vtkErrorCode::NoFileNameError;
  }

  error_code = vtkSMUtilities::SaveImage(vtkimage, filename.toUtf8().data(), quality);

  return error_code;
}

//-----------------------------------------------------------------------------
int pqImageUtil::saveImage(const QImage& qimage, const QString& filename, int quality /*=-1*/)
{
  int error_code = vtkErrorCode::NoError;
  if (qimage.isNull())
  {
    return vtkErrorCode::UnknownError;
  }

  if (filename.isEmpty())
  {
    return vtkErrorCode::NoFileNameError;
  }

  // Use VTK for saving image, so that 3 component images are saved as needed
  // by vtk for testing etc.
  vtkImageData* vtkimage = vtkImageData::New();
  if (pqImageUtil::toImageData(qimage, vtkimage))
  {
    error_code = pqImageUtil::saveImage(vtkimage, filename, quality);
  }
  else
  {
    error_code = vtkErrorCode::UnknownError;
  }

  return error_code;
}
