/*=========================================================================

  Program:   ParaView
  Module:    vtkRMScalarBarWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRM3DWidget - 
// .SECTION Description


#ifndef __vtkRMScalarBarWidget_h
#define __vtkRMScalarBarWidget_h

#include "vtkRMObject.h"

class vtkPVProcessModule;
class vtkScalarBarWidget;
class vtkLookupTable;
class vtkScalarBarWidgetObserver;

class VTK_EXPORT vtkRMScalarBarWidget : public vtkRMObject
{
public:
  static vtkRMScalarBarWidget *New();
  vtkTypeRevisionMacro(vtkRMScalarBarWidget, vtkRMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  virtual void Create(vtkPVProcessModule *pm, vtkClientServerID renderer2DID, 
    vtkClientServerID interactorID);
//ETX

  void SetVectorModeToMagnitude();
  void SetVectorModeToComponent();

//BTX
  vtkGetMacro(ScalarBarActorID,vtkClientServerID);
  vtkGetMacro(LookupTableID,vtkClientServerID);
  vtkGetObjectMacro(LookupTable,vtkLookupTable);
//ETX
  // Description:
  // The name of the color map serves as the label of the ScalarBar 
  // (e.g. Temperature). Currently it also indicates the arrays mapped
  // by this color map object.
  void SetScalarBarTitle(const char* name);
  const char* GetScalarBarTitle() {return this->ScalarBarTitle;}
  const char* GetVectorMagnitudeTitle() {return this->VectorMagnitudeTitle;}
//BTX
  enum VectorModes {
    MAGNITUDE=0,
    COMPONENT=1
  };
//ETX
  int GetVectorMode() { return this->VectorMode; }


  // Description:
  // This map is used for arrays with this name 
  // and this number of components.  In the future, they may
  // handle more than one type of array.
  void SetArrayName(const char* name);
  const char* GetArrayName() { return this->ArrayName;}
  int MatchArrayName(const char* name, int numberOfComponents);

  void SetNumberOfVectorComponents(int num);
  vtkGetMacro(NumberOfVectorComponents, int);
  char** GetVectorComponentTitles()
    { return this->VectorComponentTitles; }


  // Description:
  // The format of the scalar bar labels.
  void SetScalarBarLabelFormat(const char* name);
  vtkGetStringMacro(ScalarBarLabelFormat);

  void SetScalarBarVectorTitle(const char* name);
  const char* GetScalarBarVectorTitle();

  void SetNumberOfColors(int num);
  vtkGetMacro(NumberOfColors,int);
  
  void SetStartHSV(double h, double s, double v);
  void SetStartHSV(double hsv[3])
    {this->SetStartHSV(hsv[0],hsv[1],hsv[2]);}
  vtkGetVector3Macro(StartHSV,double);

  void SetEndHSV(double h, double s, double v);
  void SetEndHSV(double hsv[3])
    {this->SetEndHSV(hsv[0],hsv[1],hsv[2]);}
  vtkGetVector3Macro(EndHSV,double);

  void SetVectorComponent(int component);
  vtkGetMacro(VectorComponent,int);
  
  void SetScalarRange(double min, double max);
  vtkGetVector2Macro(ScalarRange,double);

  void SetScalarBarPosition1(float x, float y);
  void SetScalarBarPosition2(float x, float y);
  void SetScalarBarOrientation(int o);

  void LabToXYZ(double Lab[3], double xyz[3]);
  void XYZToRGB(double xyz[3], double rgb[3]);

  void SetVisibility(int visible);
  
//BTX

  vtkScalarBarWidget* GetScalarBar(){return this->ScalarBar;}

  // Description:
  // This method is called when event is triggered on the scalar bar.
  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event,  
                            void* calldata);
//ETX
protected:
//BTX
  vtkRMScalarBarWidget();
  ~vtkRMScalarBarWidget();
  
  vtkPVProcessModule* PVProcessModule;

  char* ArrayName;
  int NumberOfVectorComponents;
  char* ScalarBarTitle;
  char* ScalarBarLabelFormat;
  
  vtkScalarBarWidget* ScalarBar;
  vtkScalarBarWidgetObserver* ScalarBarObserver;

  char *VectorMagnitudeTitle;
  char **VectorComponentTitles;
  char *ScalarBarVectorTitle; //string for ScalarBarVectorTitleEntry

  int NumberOfColors;
  double ScalarRange[2];
  int VectorMode;
  int VectorComponent;

  double StartHSV[3];
  double EndHSV[3];

  vtkClientServerID ScalarBarActorID;
  vtkClientServerID Renderer2DID;

  void UpdateScalarBarTitle();
  //void UpdateVectorComponentMenu();
  void UpdateScalarBarLabelFormat();
  void UpdateLookupTable();

  vtkLookupTable* LookupTable;
  vtkClientServerID LookupTableID;

private:  
  vtkRMScalarBarWidget(const vtkRMScalarBarWidget&); // Not implemented
  void operator=(const vtkRMScalarBarWidget&); // Not implemented
//ETX
};

#endif
