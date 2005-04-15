/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVolumePropertyWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVVolumePropertyWidget - 
//
// .SECTION Description
// Just for passing pvinformation instead of directly the dataset

#ifndef __vtkPVVolumePropertyWidget_h
#define __vtkPVVolumePropertyWidget_h

#include "vtkKWVolumePropertyWidget.h"

class vtkPVDataInformation;
class VTK_EXPORT vtkPVVolumePropertyWidget : public vtkKWVolumePropertyWidget
{
public:
  static vtkPVVolumePropertyWidget* New();
  vtkTypeRevisionMacro(vtkPVVolumePropertyWidget, vtkKWVolumePropertyWidget);
  void PrintSelf(ostream &os, vtkIndent indent);
  
//BTX
  vtkGetObjectMacro(DataInformation, vtkPVDataInformation);
  virtual void SetDataInformation(vtkPVDataInformation*);
//ETX

  // Description:
  // Set the array name that's being used for volume rendering.
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);

  // Description:
  // Set the Scalar mode (whther to use Point Field Data or Cell Field Data).
  void SetScalarModeToUsePointFieldData() 
    { this->ScalarMode = vtkPVVolumePropertyWidget::POINT_FIELD_DATA; }
  void SetScalarModeToUseCellFieldData() 
    { this->ScalarMode = vtkPVVolumePropertyWidget::CELL_FIELD_DATA; }
  vtkGetMacro(ScalarMode, int);

  void SetDataSet(vtkDataSet*) 
    { vtkErrorMacro( << "Don't use this method"); };

  //BTX
  enum { POINT_FIELD_DATA = 0, CELL_FIELD_DATA };
  //ETX
 
protected:
  vtkPVVolumePropertyWidget();
  ~vtkPVVolumePropertyWidget();
  
  vtkPVDataInformation *DataInformation;

  // Get the dataset information
  // This methods will be overriden in subclasses so that something
  // different than the DataSet ivar will be used to compute the
  // corresponding items
  virtual int GetDataSetNumberOfComponents();
  virtual int GetDataSetScalarRange(int comp, double range[2]);
  virtual int GetDataSetAdjustedScalarRange(int comp, double range[2]);
  virtual const char* GetDataSetScalarName();
  virtual int GetDataSetScalarOpacityUnitDistanceRangeAndResolution(
    double range[2], double *resolution);
 
  char* ArrayName;
  int ScalarMode;
private:
  vtkPVVolumePropertyWidget(const vtkPVVolumePropertyWidget&);  // Not implemented
  void operator=(const vtkPVVolumePropertyWidget&);  // Not implemented
};

#endif
