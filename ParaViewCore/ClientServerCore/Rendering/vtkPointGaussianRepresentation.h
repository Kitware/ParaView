/*=========================================================================

  Program:   ParaView
  Module:    vtkPointGaussianRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointGaussianRepresentation
 *
 * Representation for showing point data as sprites, including gaussian
 * splats, spheres, or some custom shaded representation.
*/

#ifndef vtkPointGaussianRepresentation_h
#define vtkPointGaussianRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" // needed for exports
#include "vtkPVDataRepresentation.h"
#include "vtkSmartPointer.h" // needed for smart pointer
#include <string>            // for std::string
#include <vector>            // for std::vector

class vtkActor;
class vtkDataObject;
class vtkPiecewiseFunction;
class vtkPointGaussianMapper;
class vtkScalarsToColors;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPointGaussianRepresentation
  : public vtkPVDataRepresentation
{
public:
  vtkTypeMacro(vtkPointGaussianRepresentation,
    vtkPVDataRepresentation) static vtkPointGaussianRepresentation* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  /**
   * Use to set the color map for the data in this representation
   */
  void SetLookupTable(vtkScalarsToColors* lut);

  /**
   * Use to set whether the data in this representation is visible or not
   */
  void SetVisibility(bool val) override;

  /**
   * Use to set whether the splat emits light
   */
  virtual void SetEmissive(bool val);

  /**
   * Use to make sure scalars will be mapped through lookup table, and not
   * inadvertently used as colors by themselves.
   */
  virtual void SetMapScalars(int val);

  //***************************************************************************
  // Forwarded to Actor.
  virtual void SetOrientation(double, double, double);
  virtual void SetOrigin(double, double, double);
  virtual void SetPickable(int val);
  virtual void SetPosition(double, double, double);
  virtual void SetScale(double, double, double);

  //***************************************************************************
  // Forwarded to Actor->GetProperty()
  virtual void SetAmbientColor(double r, double g, double b);
  virtual void SetColor(double r, double g, double b);
  virtual void SetDiffuseColor(double r, double g, double b);
  virtual void SetEdgeColor(double r, double g, double b);
  virtual void SetInterpolation(int val);
  virtual void SetLineWidth(double val);
  virtual void SetOpacity(double val);
  virtual void SetPointSize(double val);
  virtual void SetSpecularColor(double r, double g, double b);
  virtual void SetSpecularPower(double val);

  /**
   * Sets the radius of the gaussian splats if there is no scale array or if
   * the scale array is disabled.  Defaults to 1.
   */
  virtual void SetSplatSize(double radius);

  /**
   * An enum specifying some preset fragment shaders
   */
  enum ShaderPresets
  {
    GAUSSIAN_BLUR, // This is the default
    SPHERE,        // Points shaded to look (something) like a sphere lit from the view direction
    BLACK_EDGED_CIRCLE, // Camera facing, flat circle, rimmed in black
    PLAIN_CIRCLE,       // Same as above, but without the black edge
    TRIANGLE,           // Camera facing, flat triangle
    SQUARE_OUTLINE,     // Camera facing, flat square, with empty center
    CUSTOM,             // Custom shader
    NUMBER_OF_PRESETS   // !!! THIS MUST ALWAYS BE THE LAST PRESET ENUM !!!
  };

  /**
   * Allows to select one of several preset options for shading the points
   */
  void SelectShaderPreset(int preset);

  /**
   * Sets the snippet of fragment shader code used to color the sprites.
   */
  void SetCustomShader(const char* shaderString);

  /**
   * Sets the scale of the triangle geometry drawn for the custom shader
   */
  void SetCustomTriangleScale(double scale);

  /**
   * Sets the point array to scale the guassians by.  The array should be a
   * float array.  The first four parameters are unused and only needed for
   * the ParaView GUI's signature recognition.
   */
  void SelectScaleArray(int, int, int, int, const char* name);

  /**
   * Sets the point array component to scale the gaussians by.
   */
  void SelectScaleArrayComponent(int component);

  /**
   * Use scale transfer function. If false, no mapping is done.
   */
  void SetUseScaleFunction(bool enable);

  /**
   * Sets a vtkPiecewiseFunction to use in mapping array values to sprite
   * sizes.  Performance decreases (along with understandability) when
   * large values are used for sprite sizes.  This is only used when
   * "SetScaleArray" is also set.
   */
  void SetScaleTransferFunction(vtkPiecewiseFunction* pwf);

  /**
   * Sets a vtkPiecewiseFunction to use in mapping array values to sprite
   * opacities.  Only used when "Opacity Array" is set.
   */
  void SetOpacityTransferFunction(vtkPiecewiseFunction* pwf);

  /**
   * Sets the point array to use in calculating point sprite opacities.
   * The array should be a float or double array.  The first four
   * parameters are unused and only needed for the ParaView GUI's
   * signature recognition.
   */
  void SelectOpacityArray(int, int, int, int, const char* name);

  /**
   * Sets the point array component to opacify the gaussians with.
   */
  void SelectOpacityArrayComponent(int component);

  //@{
  /**
   * Enables or disables setting opacity by an array.  Set which array
   * should be used for opacity with SelectOpacityArray, and set an
   * opacity transfer function with SetOpacityTransferFunction.
   */
  void SetOpacityByArray(bool newVal);
  vtkGetMacro(OpacityByArray, bool);
  vtkBooleanMacro(OpacityByArray, bool);
  //@}

  //@{
  /**
   * Enables or disables scaling by a data array vs. a constant factor.  Set
   * which data array with SelectScaleArray and SetSplatSize.
   */
  void SetScaleByArray(bool newVal);
  vtkGetMacro(ScaleByArray, bool);
  vtkBooleanMacro(ScaleByArray, bool);
  //@}

protected:
  vtkPointGaussianRepresentation();
  ~vtkPointGaussianRepresentation() override;

  bool AddToView(vtkView* view) override;
  bool RemoveFromView(vtkView* view) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void UpdateColoringParameters();
  vtkSetStringMacro(LastScaleArray);
  vtkSetStringMacro(LastOpacityArray);
  void InitializeShaderPresets();
  void UpdateMapperScaleFunction();

  vtkSmartPointer<vtkActor> Actor;
  vtkSmartPointer<vtkPointGaussianMapper> Mapper;
  vtkSmartPointer<vtkDataObject> ProcessedData;
  vtkSmartPointer<vtkPiecewiseFunction> ScaleFunction;

  int SelectedPreset;

  bool ScaleByArray;
  char* LastScaleArray;
  int LastScaleArrayComponent;

  bool OpacityByArray;
  char* LastOpacityArray;
  int LastOpacityArrayComponent;

  bool UseScaleFunction;

  std::vector<std::string> PresetShaderStrings;
  std::vector<float> PresetShaderScales;

private:
  vtkPointGaussianRepresentation(const vtkPointGaussianRepresentation&) = delete;
  void operator=(const vtkPointGaussianRepresentation&) = delete;
};

#endif // vtkPointGaussianRepresentation_h
