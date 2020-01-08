/*=========================================================================

  Program:   ParaView
  Module:    vtkRawImageFileSeriesReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRawImageFileSeriesReader
 * @brief   adds support for optionally reading raw image stacks.
 *
 * vtkRawImageFileSeriesReader is designed to read in raw files. The issue
 * with raw files is that the extents are not known and must be passed to
 * vtkImageReader2 and subclasses.
*/

#ifndef vtkRawImageFileSeriesReader_h
#define vtkRawImageFileSeriesReader_h

#include "vtkImageFileSeriesReader.h"
#include "vtkPVVTKExtensionsIOImageModule.h" //needed for exports

class VTKPVVTKEXTENSIONSIOIMAGE_EXPORT vtkRawImageFileSeriesReader : public vtkImageFileSeriesReader
{
public:
  static vtkRawImageFileSeriesReader* New();
  vtkTypeMacro(vtkRawImageFileSeriesReader, vtkImageFileSeriesReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The number of dimensions stored in a file. This defaults to two.
   */
  vtkSetClampMacro(FileDimensionality, int, 2, 3);
  vtkGetMacro(FileDimensionality, int);
  //@}

  //@{
  /**
   * Get/Set the extent of the data on disk.
   */
  vtkSetVector6Macro(DataExtent, int);
  vtkGetVector6Macro(DataExtent, int);
  //@}

protected:
  vtkRawImageFileSeriesReader();
  ~vtkRawImageFileSeriesReader();

  //@{
  /**
   * Update the reader extent if the image file format does not know
   * what it is (e.g. the raw format). Here we pass in values set by the user.
   */
  virtual void UpdateReaderDataExtent() override;
  //@}

  //@{
  /**
   * Raw files don't have any extent information in them and are required
   * to be set by the user. If we are reading a 2D raw image as a stack
   * we fill in the 3rd extent automatically for the reader actually reading
   * the files and ignore the 3rd extent here. We do need to keep that extent
   * in case the user switches between reading as a stack or reading as a
   * time series.
   */
  int DataExtent[6];
  //@}

  //@{
  /**
   * The dimensionality that we pass to the actual image reader.
   * Default is 2.
   */
  int FileDimensionality;
  //@}

private:
  vtkRawImageFileSeriesReader(const vtkRawImageFileSeriesReader&) = delete;
  void operator=(const vtkRawImageFileSeriesReader&) = delete;
};

#endif
