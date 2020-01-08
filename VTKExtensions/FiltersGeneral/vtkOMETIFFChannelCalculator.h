/*=========================================================================

  Program:   ParaView
  Module:    vtkOMETIFFChannelCalculator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkOMETIFFChannelCalculator
 * @brief filter to combine multiple channels into colors
 *
 * vtkOMETIFFChannelCalculator is designed to combine multiple channels,
 * typically from an OME-TIFF file, to generate RGBA colors. Up to 10 channels
 * are supported. One must specify a `vtkScalarsToColors` object to use to map
 * the channel scalars to colors for each enabled channel.
 */

#ifndef vtkOMETIFFChannelCalculator_h
#define vtkOMETIFFChannelCalculator_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer

#include <map> // needed for map

class vtkDataArraySelection;
class vtkScalarsToColors;
class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkOMETIFFChannelCalculator
  : public vtkPassInputTypeAlgorithm
{
public:
  static vtkOMETIFFChannelCalculator* New();
  vtkTypeMacro(vtkOMETIFFChannelCalculator, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Choose the channels to combine in this calculator.
   */
  vtkGetObjectMacro(ChannelSelection, vtkDataArraySelection);
  //@}

  //@{
  /**
   * Set the transfer functions to use to map scalars to colors.
   */
  void SetChannel1LUT(vtkScalarsToColors* stc) { this->SetLUT("Channel_1", stc); }
  void SetChannel2LUT(vtkScalarsToColors* stc) { this->SetLUT("Channel_2", stc); }
  void SetChannel3LUT(vtkScalarsToColors* stc) { this->SetLUT("Channel_3", stc); }
  void SetChannel4LUT(vtkScalarsToColors* stc) { this->SetLUT("Channel_4", stc); }
  void SetChannel5LUT(vtkScalarsToColors* stc) { this->SetLUT("Channel_5", stc); }
  void SetChannel6LUT(vtkScalarsToColors* stc) { this->SetLUT("Channel_6", stc); }
  void SetChannel7LUT(vtkScalarsToColors* stc) { this->SetLUT("Channel_7", stc); }
  void SetChannel8LUT(vtkScalarsToColors* stc) { this->SetLUT("Channel_8", stc); }
  void SetChannel9LUT(vtkScalarsToColors* stc) { this->SetLUT("Channel_9", stc); }
  void SetChannel10LUT(vtkScalarsToColors* stc) { this->SetLUT("Channel_10", stc); }
  void SetLUT(const char* channelName, vtkScalarsToColors*);
  //@}

  //@{
  /**
   * Set weights for each of the channels.
   */
  void SetChannel1Weight(double wght) { this->SetWeight("Channel_1", wght); }
  void SetChannel2Weight(double wght) { this->SetWeight("Channel_2", wght); }
  void SetChannel3Weight(double wght) { this->SetWeight("Channel_3", wght); }
  void SetChannel4Weight(double wght) { this->SetWeight("Channel_4", wght); }
  void SetChannel5Weight(double wght) { this->SetWeight("Channel_5", wght); }
  void SetChannel6Weight(double wght) { this->SetWeight("Channel_6", wght); }
  void SetChannel7Weight(double wght) { this->SetWeight("Channel_7", wght); }
  void SetChannel8Weight(double wght) { this->SetWeight("Channel_8", wght); }
  void SetChannel9Weight(double wght) { this->SetWeight("Channel_9", wght); }
  void SetChannel10Weight(double wght) { this->SetWeight("Channel_10", wght); }
  void SetWeight(const char* channelName, double weight);
  //@}
  vtkMTimeType GetMTime() override;

protected:
  vtkOMETIFFChannelCalculator();
  ~vtkOMETIFFChannelCalculator() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  struct LUTItem
  {
    vtkSmartPointer<vtkScalarsToColors> LUT;
    double Weight;
  };

  std::map<std::string, LUTItem> LUTMap;
  vtkDataArraySelection* ChannelSelection;

private:
  vtkOMETIFFChannelCalculator(const vtkOMETIFFChannelCalculator&) = delete;
  void operator=(const vtkOMETIFFChannelCalculator&) = delete;
};

#endif
