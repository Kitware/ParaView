/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPartDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPartDisplay - Superclass for actor/mapper control.
// .SECTION Description
// This is a superclass for objects which display PVParts.
// vtkPVRenderModules create displays.  This class is not meant to be
// used directly, but it implements the simplest serial display
// which has no levels of detail.

#ifndef __vtkSMPartDisplay_h
#define __vtkSMPartDisplay_h

// A couple of additional representations that vtkProperty does not handle.
#define VTK_OUTLINE 111
#define VTK_VOLUME 222

#include "vtkSMDisplay.h"

class vtkClientServerStream;
class vtkSMProxy;
class vtkDataSet;
class vtkPVDataInformation;
class vtkSMPart;
class vtkSMSourceProxy;
class vtkSMIntVectorProperty;
class vtkSMDoubleVectorProperty;
class vtkVolume;
class vtkVolumeProperty;
class vtkPiecewiseFunction;
class vtkColorTransferFunction;
class vtkUnstructuredGridVolumeRayCastMapper;
class vtkPVArrayInformation;
class vtkPVProcessModule;
class vtkPVGeometryInformation;

class VTK_EXPORT vtkSMPartDisplay : public vtkSMDisplay
{
public:
  static vtkSMPartDisplay* New();
  vtkTypeRevisionMacro(vtkSMPartDisplay, vtkSMDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Connect the VTK data object to the display pipeline.
  virtual void SetInput(vtkSMSourceProxy* input);

  // Description:
  // This method updates the piece that has been assigned to this process.
  virtual void Update();

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);  

  // Description:
  // Toggles the mappers to use immediate mode rendering or display lists.
  virtual void SetUseImmediateMode(int val);

  // Description:
  // Turns visibility on or off.
  virtual void SetVisibility(int v);
  vtkGetMacro(Visibility,int);

  // Description:
  // Change the color mode to map scalars or 
  // use unsigned char arrays directly.
  // MapScalarsOff only works when coloring by an array 
  // on unsigned chars with 1 or 3 components.
  // The colorMap proxy must be vtkSMLookupTableProxy
  virtual void SetDirectColorFlag(int val);
  int GetDirectColorFlag();
  virtual void SetScalarVisibility(int val);
  int GetScalarVisibility();
  virtual void ColorByArray(vtkSMProxy* colorMap, int field);
  vtkGetMacro(ColorField,int);

  // Description:
  // Temporarily place holder for properties.
  void SetLineWidth(double w);
  double GetLineWidth();
  void SetPointSize(double w);
  double GetPointSize();
  void SetInterpolation(int interpolation);
  int GetInterpolation();
  void SetOpacity(double opacity);
  double GetOpacity();
  void SetTranslate(double x, double y, double z);
  void GetTranslate(double* translate);
  void SetScale(double x, double y, double z);
  void GetScale(double* scale);
  void SetOrientation(double x, double y, double z);
  void GetOrientation(double* oreination);
  void SetOrigin(double x, double y, double z);
  void GetOrigin(double* origin);

  // Description:
  // This just sets the color of the property.
  // you also have to set scalar visiblity to off.
  virtual void SetColor(float r, float g, float b);
  virtual void GetColor(float *rgb);

  void SetRepresentation(int rep);
  vtkGetMacro(Representation,int);
  vtkGetMacro(OpacityUnitDistance, double)
  
  // Description:
  // Option to use a 1d texture map for the attribute color.
  virtual void SetInterpolateColorsFlag(int val);
  int GetInterpolateColorsFlag();

  // Description:
  // Tells the geometry filter to strip its output.
  virtual void SetUseTriangleStrips(int val);
  int GetUseTriangleStrips();


  //BTX
  // Description:
  // It would be nice to get rid of access toi these objects in the API.
  vtkGetObjectMacro(VolumeProxy, vtkSMProxy);
  vtkGetObjectMacro(VolumeOpacityProxy, vtkSMProxy);
  vtkGetObjectMacro(VolumeColorProxy, vtkSMProxy);
  vtkGetObjectMacro(VolumePropertyProxy, vtkSMProxy);
  vtkPiecewiseFunction *GetVolumeOpacity() {return this->VolumeOpacity;};
  vtkColorTransferFunction *GetVolumeColor() {return this->VolumeColor;};
  //ETX
  
  // Description:
  // PVSource calls this when it gets modified.
  virtual void InvalidateGeometry();

  // Description:
  // Select a point field to use for volume rendering
  virtual void VolumeRenderPointField(const char *name);

  // Description:
  // Select a cell field to use for volume rendering
  virtual void VolumeRenderCellField(const char *name);

  // Description:
  // Turn on/off volume rendering. This controls which prop is 
  // visible since both geometric and volumetric pipelines exist
  // simultaneously.
  virtual void VolumeRenderModeOn();
  virtual void VolumeRenderModeOff();

  // Description:
  // Return whether volume rendering is on or off.
  vtkGetMacro(VolumeRenderMode, int);

  // Description:
  // Set/Get the scalar array used during volume rendering.
  vtkSetStringMacro(VolumeRenderField);
  vtkGetStringMacro(VolumeRenderField);

  // Description:
  // Initialize the transfer functions based on the scalar range
  void ResetTransferFunctions(vtkPVArrayInformation *arrayInfo,
                              vtkPVDataInformation *dataInfo);
  void InitializeTransferFunctions(vtkPVArrayInformation *arrayInfo, 
                                   vtkPVDataInformation *dataInfo);
  // Description:
  // A reference to the part is needed because we have to check the type 
  // of the vtkDataObject.  Volume rendering and Label IDs need it.
  vtkSMSourceProxy* GetSource() {return this->Source;}

  // Description:
  // Until the proxies do this.
  void SaveInBatchScript(ofstream *file, vtkSMSourceProxy* pvs);
  void SaveGeometryInBatchFile(ofstream *file, const char* filename,
                               int timeIdx); 
  
//BTX  
  // Description:
  // Temporary solution to write geometry from animation editor.
  void ConnectGeometryForWriting(vtkClientServerID consumerID,
                                 const char* methodName,
                                 vtkClientServerStream* stream);
  void AddToRenderer(vtkPVRenderModule*);
  void RemoveFromRenderer(vtkPVRenderModule*);
                                                    
  // Description:
  // Get data information from the geometry filters output.
  vtkPVGeometryInformation* GetGeometryInformation();
//ETX

  // Description:
  // Saves the pipeline in a ParaView script.  This is similar
  // to saveing a trace, except only the last state is stored.
  virtual void SavePVState(ostream *file, const char* tclName, 
                           vtkIndent indent);

protected:
  vtkSMPartDisplay();
  ~vtkSMPartDisplay();
  
  int GeometryInformationIsValid;
  void GatherGeometryInformation();
  vtkPVGeometryInformation* GeometryInformation;

  // Description:
  // A reference to the part is needed because we have to check the type 
  // of the vtkDataObject.  Volume rendering and Label IDs need it.
  void SetSource(vtkSMSourceProxy* source) {this->Source = source;}

  virtual void CreateVTKObjects(int num);
  virtual void RemoveAllCaches();

  // Description:
  // Sends the current stream to the client and server. 
  void SendForceUpdate(vtkClientServerStream* str);

  void SetInputInternal(vtkSMSourceProxy *input, vtkPVProcessModule *pm);

  vtkSMSourceProxy* Source;
  vtkSMProxy* ColorMap;
  
  vtkSMIntVectorProperty* ScalarVisibilityProperty;
  vtkSMIntVectorProperty* DirectColorFlagProperty;
  vtkSMIntVectorProperty* InterpolateColorsFlagProperty;
  vtkSMIntVectorProperty* PropVisibilityProperty;
  vtkSMIntVectorProperty* VolumeVisibilityProperty;
  vtkSMIntVectorProperty* UseTriangleStripsProperty;
  vtkSMIntVectorProperty* UseImmediateModeProperty;
  vtkSMDoubleVectorProperty* LineWidthProperty;
  vtkSMDoubleVectorProperty* PointSizeProperty;
  vtkSMIntVectorProperty* InterpolationProperty;
  
  vtkSMDoubleVectorProperty* OpacityProperty;
  vtkSMDoubleVectorProperty* TranslateProperty;
  vtkSMDoubleVectorProperty* ScaleProperty;
  vtkSMDoubleVectorProperty* OrientationProperty;
  vtkSMDoubleVectorProperty* OriginProperty;
  vtkSMDoubleVectorProperty* ColorProperty;


  vtkSMProxy* GeometryProxy;
  vtkSMProxy* PropProxy;
  vtkSMProxy* PropertyProxy;
  vtkSMProxy* MapperProxy;
  vtkSMProxy* UpdateSuppressorProxy;

  vtkSMProxy* VolumeProxy;
  vtkSMProxy* VolumePropertyProxy;
  vtkSMProxy* VolumeMapperProxy;
  vtkSMProxy* VolumeOpacityProxy;
  vtkSMProxy* VolumeColorProxy;
  vtkSMProxy* VolumeTetraFilterProxy;

  // These are too complex to make into a property for internal use.
  int    Representation;
  int    Visibility;
  double OpacityUnitDistance;
  int    ColorField;

  vtkVolume                *Volume;
  vtkPiecewiseFunction     *VolumeOpacity;
  vtkColorTransferFunction *VolumeColor;
  
  int GeometryIsValid;
  int VolumeRenderMode;

  char *VolumeRenderField;

  vtkSMPartDisplay(const vtkSMPartDisplay&); // Not implemented
  void operator=(const vtkSMPartDisplay&); // Not implemented
};

#endif
