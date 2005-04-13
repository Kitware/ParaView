/*=========================================================================

  Module:    vtkKWHistogram.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWHistogramSet - an histogram
// .SECTION Description

#ifndef __vtkKWHistogram_h
#define __vtkKWHistogram_h

#include "vtkObject.h"

class vtkColorTransferFunction;
class vtkDataArray;
class vtkImageData;
class vtkDoubleArray;

class VTK_EXPORT vtkKWHistogram : public vtkObject
{
public:
  static vtkKWHistogram* New();
  vtkTypeRevisionMacro(vtkKWHistogram,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the histogram range.
  // Note that Range[1] is exclusive.
  // The Range is updated automatically by the BuildHistogram method to
  // match the range of the vtkDataArray passed as parameter.
  vtkGetVector2Macro(Range, double);

  // Description:
  // Direct access to the bins
  vtkGetObjectMacro(Bins, vtkDoubleArray);

  // Description:
  // Convenience method to get the number of bins
  virtual vtkIdType GetNumberOfBins();

  // Description:
  // Set/Get the maximum number of bins that should be used when
  // creating the histogram
  vtkSetMacro(MaximumNumberOfBins, vtkIdType);
  vtkGetMacro(MaximumNumberOfBins, vtkIdType);

  // Description:
  // Convenience method to get min, max, total occurence
  virtual double GetMinimumOccurence();
  virtual double GetMaximumOccurence();
  virtual double GetTotalOccurence();

  // Description:
  // Get the occurence for the bin holding a given value.
  virtual double GetOccurenceAtValue(double value);

  // Description:
  // Get the value at a given accumulated occurence in the histogram.
  // 'exclude_value' is not NULL, it is a pointer to a value which bin
  // will be ignored from the computation.
  virtual double GetValueAtAccumulatedOccurence(
    double acc, double *exclude_value = 0);

  // Description:
  // Build/update the histogram from scalars (given a component)
  // The Range and number of bins are modified automatically
  virtual void BuildHistogram(vtkDataArray *scalars, int component);

  // Description:
  // Set the histogram range.
  // Note that Range[1] is exclusive.
  // The Range is updated automatically by the BuildHistogram method to
  // match the range of the vtkDataArray passed as parameter.
  // Nevertheless, you might want to set the Range manually, either after
  // calling the BuildHistogram method to restrict the histogram to a subset, 
  // or before calling the AccumulateHistogram method, which accumulates
  // values and update the histogram.
  // Use the EstimateHistogramRange method to compute the range that
  // is needed to store a set of scalars.
  vtkSetVector2Macro(Range, double);

  // Description:
  // Estimate the range that will be used by BuildHistogram from scalars
  // (given a component)
  virtual void EstimateHistogramRange(
    vtkDataArray *scalars, int component, double range[2]);

  // Description:
  // Accumulate the histogram from scalars (given a component)
  // The Range is *not* modified automatically, you have to set it 
  // appropriately before calling this method using either:
  //   - a call to SetRange followed by a call to EmptyHistogram
  //     (use the EstimateHistogramRange method to compute the range that
  //      is needed to store a set of scalars)
  //   - or a call to BuildHistogram first, with the appropriate 'scalars'. 
  // This method can be called multiple times with a different 'scalars':
  //   - the 'scalars' range has to lie within the Range ivar. 
  //   - the bins are not reset to 0, the values of 'scalars' are used to
  //     update the histogram with more data (accumulate)
  //     => EmptyHistogram will reset the histogram (remove all bins,
  //        forcing all bins to be set to 0 the next time the number
  //        of bins is changed).
  virtual void AccumulateHistogram(vtkDataArray *scalars, int component);

  // Description:
  // Empty this histogram (0 bins). The next time the number of bins is
  // changed (BuildHistogram or AccumulateHistogram), each bin is set to 0.
  virtual void EmptyHistogram();

  // Description:
  // Compute the image of the histogram in log space (default).
  virtual void SetLogMode(int);
  vtkBooleanMacro(LogMode, int);
  vtkGetMacro(LogMode, int);

protected:
  vtkKWHistogram();
  ~vtkKWHistogram();

  double Range[2];

  vtkDoubleArray *Bins;

  vtkImageData *Image;
  unsigned long LastImageBuildTime;
  unsigned long LastTransferFunctionTime;
  int           LogMode;
  vtkIdType     MaximumNumberOfBins;

  virtual void ComputeStatistics();
  unsigned long LastStatisticsBuildTime;

  double MinimumOccurence;
  double MaximumOccurence;
  double TotalOccurence;

  // Description:
  // Update the histogram from scalars (given a component)
  // Either reset the range (BuildHistogram) or not (AccumulateHistogram)
  virtual void UpdateHistogram(
    vtkDataArray *scalars, int component, int reset_range);

  // Description:
  // Estimate the range that will be used by BuildHistogram from scalars
  // (given a component), and the number of bins
  virtual void EstimateHistogramRangeAndNumberOfBins(
    vtkDataArray *scalars, int component, 
    double range[2], vtkIdType *nb_of_bins);

public:
  // Description:
  // Get an image of the histogram. The image parameters are described 
  // through an instance of the above ImageDescriptor. The histogram
  // can be drawn for a given data range.
  // If DrawBackground is false, the background pixels are set to be 
  // transparent and the resulting image is created in RGBA space instead
  // of RGB.
  //BTX
  class VTK_EXPORT ImageDescriptor
  {
  public:
    ImageDescriptor();

    int  IsValid() const;
    int  IsEqualTo(const ImageDescriptor *desc);
    void Copy(const ImageDescriptor *desc);

    int          DrawBackground;
    int          DrawGrid;
    int          GridSize;

    enum
    {
      STYLE_BARS = 0,
      STYLE_DOTS
    };
    int          Style;

    double       Range[2];
    void SetRange(double range0, double range1);
    void SetRange(double range[2]);

    unsigned int Width;
    unsigned int Height;
    void SetDimensions(unsigned int width, unsigned int height);

    double       Color[3];
    void SetColor(double color[3]);

    double       BackgroundColor[3];
    void SetBackgroundColor(double color[3]);

    double       OutOfRangeColor[3];
    void SetOutOfRangeColor(double color[3]);

    double       GridColor[3];
    void SetGridColor(double color[3]);

    vtkColorTransferFunction *ColorTransferFunction;

    // When the histogram is drawn, a resampled version of the histogram
    // is computed where each column-pixel along the Width is a bin.
    // The histogram itself is scaled vertically so that the bin with
    // the maximum occurence occupies the full Height.
    // Each time it occurs, the LastMaximumOccurence ivar is modified
    // to store this maximum occurence.
    // If the DefaultMaximumOccurence is set to something other than 0.0
    // it will override the maximum occurence found so far and be used
    // to scale the histogram vertically (of course, it has to be >=
    // to the current maximum occurence).
    // Using DefaultMaximumOccurence is right now the way to have two
    // histograms being scaled the same way, under some constraints.

    double       DefaultMaximumOccurence;
    double       LastMaximumOccurence;
  };
  virtual int IsImageUpToDate(const ImageDescriptor *desc = 0);
  virtual vtkImageData* GetImage(ImageDescriptor *desc);
  //ETX
  
protected:
  //BTX
  vtkKWHistogram::ImageDescriptor *LastImageDescriptor;
  //ETX

private:
  vtkKWHistogram(const vtkKWHistogram&); // Not implemented
  void operator=(const vtkKWHistogram&); // Not implemented
};

#endif

