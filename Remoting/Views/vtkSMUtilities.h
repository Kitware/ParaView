/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMUtilities
 * @brief   collection of utility methods.
 *
 * vtkSMUtilities defines a collection of useful static methods.
*/

#ifndef vtkSMUtilities_h
#define vtkSMUtilities_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMObject.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer
#include <vector>            // needed for std::vector

class vtkImageData;
class vtkPoints;
class VTKREMOTINGVIEWS_EXPORT vtkSMUtilities : public vtkSMObject
{
public:
  static vtkSMUtilities* New();
  vtkTypeMacro(vtkSMUtilities, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Save the image to a file.
   * The file is created on the process on which this method is called.
   * Return vtkErrorCode::NoError (0) on success, otherwise returns the error
   * code.
   * / quality [0,100] -- 0 = low, 100=high, -1=default
   */
  static int SaveImage(vtkImageData* image, const char* filename, int quality);
  static int SaveImage(vtkImageData* image, const char* filename)
  {
    return vtkSMUtilities::SaveImage(image, filename, -1);
  }

  /**
   * Save the image to a file using a vtkImageWriter subclass given by writerName.
   * The file is created on the process on which this method is called.
   */
  static int SaveImage(vtkImageData* image, const char* filename, const char* writerName);

  /**
   * Calls SaveImage(image, filename, writerName) only on process 0.
   * Other processes will receive the return code through a broadcast.
   */
  static int SaveImageOnProcessZero(
    vtkImageData* image, const char* filename, const char* writerName);

  /**
   * Returns the points an orbit to revolve around the \c center at a distance
   * of \c radius in the plane defined by the \c center and the \c normal. The
   * number of points returned is equal to \c resolution.
   * Returns a new instance of vtkPoints. The caller is responsible for freeing
   * the allocated memory.
   */
  static vtkPoints* CreateOrbit(
    const double center[3], const double normal[3], int resolution, const double startPoint[3]);

  // Will pick an arbitrary starting point
  static vtkPoints* CreateOrbit(
    const double center[3], const double normal[3], double radius, int resolution);

  /**
   * Convenience method used to merge a smaller image (\c src) into a
   * larger one (\c dest). The location of the smaller image in the larger image
   * are determined by their extents.
   */
  static void Merge(vtkImageData* dest, vtkImageData* src, int borderWidth = 0,
    const unsigned char* borderColorRGB = nullptr);

  /**
   * Merges multiple images into a single one and returns that.
   */
  static vtkSmartPointer<vtkImageData> MergeImages(
    const std::vector<vtkSmartPointer<vtkImageData> >& images, int borderWidth = 0,
    const unsigned char* borderColorRGB = nullptr);

  /**
   * Fill the specified extents in the image with the given color.
   * If the image is a 4 component image, then this method fills the 4th
   * component with 0xff.
   */
  static void FillImage(vtkImageData* image, const int extent[6], const unsigned char rgb[3]);

protected:
  vtkSMUtilities() {}
  ~vtkSMUtilities() override {}

private:
  vtkSMUtilities(const vtkSMUtilities&) = delete;
  void operator=(const vtkSMUtilities&) = delete;
};

#endif
