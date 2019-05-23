/*=========================================================================

   Program: ParaView
   Module:    pqImageUtil.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _pqImageUtil_h
#define _pqImageUtil_h

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
