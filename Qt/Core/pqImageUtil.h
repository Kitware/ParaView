// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqImageUtil_h
#define pqImageUtil_h

#include "pqCoreModule.h"

class QByteArray;
class QImage;
class QString;
class vtkImageData;

/**
 * Utility class to convert VTK images to Qt images and vice versa
 */
class PQCORE_EXPORT pqImageUtil
{
public:
  /**
   * convert a QImage to a vtkImageData of type VTK_UNSIGNED_CHAR. Use RGB(A) format.
   */
  static bool toImageData(const QImage& img, vtkImageData* vtkimage);

  /**
   * convert vtkImageData of type VTK_UNSIGNED_CHAR to QImage
   * Z axis is ignored. Use RGB for 3 components data, RGBA for 4-components data.
   */
  static bool fromImageData(vtkImageData* vtkimage, QImage& img);

  /**
   * Convert vtkImageData of type VTK_UNSIGNED_CHAR to a QByteArray containing a buffered version of
   * the image saved in the given format. Z axis is ignored. Use RGB for 3 components data, RGBA for
   * 4-components data. Available `format` values are given by
   * QImageWriter::supportedImageFormats().
   */
  static bool imageDataToFormatedByteArray(
    vtkImageData* vtkimage, QByteArray& bArray, const char* format);

  /**
   * Save an image to a file. Determines the type of the file using the file
   * extension. Returns the vtkErrorCode on error (vtkErrorCode::NoError i.e. 0
   * if file is successfully saved).
   * quality [0,100] -- 0 = low, 100=high, -1=default
   */
  static int saveImage(vtkImageData* vtkimage, const QString& filename, int quality = -1);

  /**
   * Save an image to a file. Determines the type of the file using the file
   * extension. Returns the vtkErrorCode on error (vtkErrorCode::NoError i.e. 0
   * if file is successfully saved).
   * quality [0,100] -- 0 = low, 100=high, -1=default
   */
  static int saveImage(const QImage& image, const QString& filename, int quality = -1);
};

#endif
