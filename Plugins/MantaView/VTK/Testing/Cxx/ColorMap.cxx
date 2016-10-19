#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkLookupTable.h"
#include "vtkMantaActor.h"
#include "vtkMantaCamera.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaProperty.h"
#include "vtkMantaRenderer.h"
#include "vtkPolyDataNormals.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"

#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

#include "vtkRegressionTestImage.h"

#ifndef usleep
#define usleep(time)
#endif

// this program tests
// various color map modes involving parameters governed by
// vtkPolyDataMapper, vtkProperty, and vtkLookupTable

//#define _MULTI_OBJECTS_
//#define _DUMPOUT_INFOR_   // no dump for formal testing

#define SLEEP_SECONDS 1
#define OBJECT_RADIUS 5.
#define OBJECT_RESOLUTION 12

#ifdef _MULTI_OBJECTS_
#define SPHERE_YCOORD (OBJECT_RADIUS * 2)
#define SPHERE_ROTATE 0.0
#else
#define SPHERE_YCOORD 0.0
#define SPHERE_ROTATE 60.
#endif

int coneVectorByVTKmapper = 1;
int sphereVectorByVTKmapper = 1;
int cylinderVectorByVTKmapper = 1;

void ConfigureMapper(vtkPolyDataMapper* mapper, double scalarMin, double scalarMax,
  int scalarVisibility, int scalarMode, int colorMode, int useLookupTableScalarRange,
  int interpolateScalarsBeforeMapping, const char* colorByArrayName,
  int colorByArrayVectorComponentIndex, int vectorByVTKmapper);
void ConfigureColorLUT(vtkLookupTable* clrLUT, double lutScalarMin, double lutScalarMax,
  double lutHueMin, double lutHueMax, int scaleMode, int vectorMode, int vectorComponentIndex);
void ConfigureProperty(vtkProperty* property, int shading, int interpolationMode,
  int representationMode, double color_r, double color_g, double color_b, double ambientCoef,
  double diffuseCoef, double specularCoef, double specularPower);
void DumpAttributeInfo(vtkPolyData* polyData, char* objName);
void DumpColorMapInfo(vtkPolyDataMapper* mapper, vtkProperty* property, int vectorByVTKmapper);

//----------------------------------------------------------------------------
int ColorMap(int argc, char* argv[])
{
// ------
// sphere

#ifdef _DUMPOUT_INFOR_
  cerr << endl << "------------ sphere ------------";
#endif

  vtkSphereSource* sphere = vtkSphereSource::New();
  sphere->SetCenter(0.0, SPHERE_YCOORD, 0.0);
  sphere->SetRadius(OBJECT_RADIUS);
  sphere->SetThetaResolution(OBJECT_RESOLUTION);
  sphere->SetPhiResolution(OBJECT_RESOLUTION);

  vtkPolyDataNormals* sphereNormals = vtkPolyDataNormals::New();
  sphereNormals->SetInputConnection(sphere->GetOutputPort());
  sphereNormals->ComputePointNormalsOn();
  sphereNormals->ComputeCellNormalsOn();
  sphereNormals->Update();

#ifdef _DUMPOUT_INFOR_
  DumpAttributeInfo(sphereNormals->GetOutput(), "Sphere");
#endif

  vtkMantaPolyDataMapper* sphereMapper = vtkMantaPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphereNormals->GetOutputPort());

  vtkLookupTable* sphereLUT = vtkLookupTable::New();
  sphereMapper->SetLookupTable(sphereLUT);

  vtkMantaActor* sphereActor = vtkMantaActor::New();
  sphereActor->SetMapper(sphereMapper);
  sphereActor->RotateY(SPHERE_ROTATE);

  vtkMantaProperty* sphereProperty = vtkMantaProperty::New();
  sphereActor->SetProperty(sphereProperty);

  ConfigureMapper(sphereMapper, -1.0, 1.0, // scalar data range
    1,                                     // scalar visibility
    VTK_SCALAR_MODE_USE_POINT_FIELD_DATA,  // scalar mode
    VTK_COLOR_MODE_MAP_SCALARS,            // color mode
    0,                                     // use LUT scalar range
    0,                                     // interp before map
    "Normals",                             // array name
    0,                                     // array component index
    sphereVectorByVTKmapper                // vector by vtkMapper
    );

  ConfigureColorLUT(sphereLUT, 0.0, 1.0, // scalar range
    0.00, 0.6,                           // hue    range
    VTK_SCALE_LINEAR,                    // scale  mode
    vtkScalarsToColors::MAGNITUDE,       // vector mode
    1                                    // component index
    );

  ConfigureProperty(sphereProperty,
    1,             // shading
    VTK_GOURAUD,   // interpolation mode
    VTK_SURFACE,   // representation mode
    0.6, 0.6, 0.6, // object color
    0.2,           // ambient  coefficient
    0.6,           // diffuse  coefficient
    0.3,           // specular coefficient
    120            // specular power
    );

#ifdef _DUMPOUT_INFOR_
  DumpColorMapInfo(sphereMapper, sphereProperty, sphereVectorByVTKmapper);
#endif

#ifdef _MULTI_OBJECTS_

// ----
// cone

#ifdef _DUMPOUT_INFOR_
  cerr << endl << "------------ cone ------------";
#endif

  vtkConeSource* cone = vtkConeSource::New();
  cone->SetRadius(OBJECT_RADIUS);
  cone->SetHeight(OBJECT_RADIUS * 2);
  cone->SetResolution(OBJECT_RESOLUTION);

  vtkPolyDataNormals* coneNormals = vtkPolyDataNormals::New();
  coneNormals->SetInputConnection(cone->GetOutputPort());
  coneNormals->ComputePointNormalsOn();
  coneNormals->ComputeCellNormalsOn();
  coneNormals->Update();

#ifdef _DUMPOUT_INFOR_
  DumpAttributeInfo(coneNormals->GetOutput(), "Cone");
#endif

  vtkMantaPolyDataMapper* coneMapper = vtkMantaPolyDataMapper::New();
  coneMapper->SetInputConnection(coneNormals->GetOutputPort());

  vtkLookupTable* coneLUT = vtkLookupTable::New();
  coneMapper->SetLookupTable(coneLUT);

  vtkMantaActor* coneActor = vtkMantaActor::New();
  coneActor->SetMapper(coneMapper);
  coneActor->AddPosition(0.0, OBJECT_RADIUS * 4, 0.0);
  coneActor->RotateZ(90.0);

  vtkMantaProperty* coneProperty = vtkMantaProperty::New();
  coneActor->SetProperty(coneProperty);

  ConfigureMapper(coneMapper, -1.0, 1.0,  // scalar data range
    1,                                    // scalar visibility
    VTK_SCALAR_MODE_USE_POINT_FIELD_DATA, // scalar mode
    VTK_COLOR_MODE_MAP_SCALARS,           // color mode
    0,                                    // use LUT scalar range
    0,                                    // interp before map
    "Normals",                            // array name
    0,                                    // array component index
    coneVectorByVTKmapper                 // vector by vtkMapper
    );

  ConfigureColorLUT(coneLUT, 0.0, 1.0, // scalar range
    0.00, 0.6,                         // hue    range
    VTK_SCALE_LINEAR,                  // scale  mode
    vtkScalarsToColors::MAGNITUDE,     // vector mode
    1                                  // component index
    );

  ConfigureProperty(coneProperty,
    1,             // shading
    VTK_GOURAUD,   // interpolation mode
    VTK_SURFACE,   // representation mode
    0.6, 0.6, 0.6, // object color
    0.2,           // ambient  coefficient
    0.6,           // diffuse  coefficient
    0.3,           // specular coefficient
    120            // specular power
    );

#ifdef _DUMPOUT_INFOR_
  DumpColorMapInfo(coneMapper, coneProperty, coneVectorByVTKmapper);
#endif

// --------
// cylinder

#ifdef _DUMPOUT_INFOR_
  cerr << endl << "------------ cylinder ------------";
#endif

  vtkCylinderSource* cylinder = vtkCylinderSource::New();
  cylinder->SetCenter(0.0, 0.0, 0.0);
  cylinder->SetRadius(OBJECT_RADIUS);
  cylinder->SetHeight(OBJECT_RADIUS * 2);
  cylinder->SetResolution(OBJECT_RESOLUTION);

  vtkPolyDataNormals* cylinderNormals = vtkPolyDataNormals::New();
  cylinderNormals->SetInputConnection(cylinder->GetOutputPort());
  cylinderNormals->ComputePointNormalsOn();
  cylinderNormals->ComputeCellNormalsOn();
  cylinderNormals->Update();

#ifdef _DUMPOUT_INFOR_
  DumpAttributeInfo(cylinderNormals->GetOutput(), "Cylinder");
#endif

  vtkMantaPolyDataMapper* cylinderMapper = vtkMantaPolyDataMapper::New();
  cylinderMapper->SetInputConnection(cylinderNormals->GetOutputPort());

  vtkLookupTable* cylinderLUT = vtkLookupTable::New();
  cylinderMapper->SetLookupTable(cylinderLUT);

  vtkMantaActor* cylinderActor = vtkMantaActor::New();
  cylinderActor->SetMapper(cylinderMapper);

  vtkMantaProperty* cylinderProperty = vtkMantaProperty::New();
  cylinderActor->SetProperty(cylinderProperty);

  ConfigureMapper(cylinderMapper, -1.0, 1.0, // scalar data range
    1,                                       // scalar visibility
    VTK_SCALAR_MODE_USE_POINT_FIELD_DATA,    // scalar mode
    VTK_COLOR_MODE_MAP_SCALARS,              // color mode
    0,                                       // use LUT scalar range
    0,                                       // interp before map
    "Normals",                               // array name
    0,                                       // array component index
    cylinderVectorByVTKmapper                // vector by vtkMapper
    );

  ConfigureColorLUT(cylinderLUT, 0.0, 1.0, // scalar range
    0.00, 0.6,                             // hue    range
    VTK_SCALE_LINEAR,                      // scale  mode
    vtkScalarsToColors::MAGNITUDE,         // vector mode
    1                                      // component index
    );

  ConfigureProperty(cylinderProperty,
    1,             // shading
    VTK_GOURAUD,   // interpolation mode
    VTK_SURFACE,   // representation mode
    0.6, 0.6, 0.6, // object color
    0.2,           // ambient  coefficient
    0.6,           // diffuse  coefficient
    0.3,           // specular coefficient
    120            // specular power
    );

#ifdef _DUMPOUT_INFOR_
  DumpColorMapInfo(cylinderMapper, cylinderProperty, cylinderVectorByVTKmapper);
#endif

#endif // _MULTI_OBJECTS_

  // ---------
  // rendering
  vtkMantaRenderer* renderer = vtkMantaRenderer::New();
  renderer->SetBackground(0.1, 0.2, 0.3);
  renderer->AddActor(sphereActor);
#ifdef _MULTI_OBJECTS_
  renderer->AddActor(coneActor);
  renderer->AddActor(cylinderActor);
#endif

  vtkMantaCamera* camera = vtkMantaCamera::New();
  renderer->SetActiveCamera(camera);
  renderer->ResetCamera();

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(static_cast<vtkRenderer*>(renderer));
  renWin->SetSize(512, 512);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  else
  {
    int index = -1;
    for (int mapperScalarMode = 3;     // 0 ~ 6
         mapperScalarMode < 5;         // 3  POINT_FIELD_DATA
         mapperScalarMode++)           // 4  CELL_FIELD_DATA
      for (int colorLutVectorMode = 0; // 0 MAGNITUDE
           colorLutVectorMode < 2;     // 1 COMPONENT
           colorLutVectorMode++)
        for (int mapperScalarVisibility = 0; mapperScalarVisibility < 2; mapperScalarVisibility++)
          for (int propertyInterpolationMode = 1; // 0 VTK_FLAT
               propertyInterpolationMode < 2;     // 1 VTK_GOURAUD
               propertyInterpolationMode++)       // 2 VTK_PHONG
            for (int propertyColor = 0; propertyColor < 2; propertyColor++)
            {
              ConfigureMapper(sphereMapper, -1.0, 1.0, // scalar range
                mapperScalarVisibility,                // scalar visibility
                mapperScalarMode,                      // type of array provided
                VTK_COLOR_MODE_MAP_SCALARS,            // MapScalars
                0,                                     // use LUT scalar range
                0,                                     // interp before mapping
                "Normals",                             // color by array name
                0,                                     // array component index
                0                                      // vtkMapper handles vector
                );
              ConfigureColorLUT(sphereLUT, 0.0, 1.0, // LUT scalar range
                0.0, 0.6,                            // LUT hue    range
                VTK_SCALE_LINEAR,                    // scale mode
                colorLutVectorMode,                  // mode to handle vector
                1                                    // vector component Id
                );
              ConfigureProperty(sphereProperty,
                1,                                   // shading
                propertyInterpolationMode,           // interpolation mode
                VTK_SURFACE,                         // VTK_POINTS / VTK_WIREFRAME
                0.6 + 0.4 * propertyColor, 0.6, 0.6, // RGB color
                0.2, 0.6, 0.3,                       // ambient, diffuse, specular
                120                                  // specular power
                );

#ifdef _MULTI_OBJECTS_
              ConfigureMapper(coneMapper, -1.0, 1.0, // scalar range
                mapperScalarVisibility,              // scalar visibility
                mapperScalarMode,                    // type of array provided
                VTK_COLOR_MODE_MAP_SCALARS,          // MapScalars
                0,                                   // use LUT scalar range
                0,                                   // interp before mapping
                "Normals",                           // color by array name
                0,                                   // array component index
                0                                    // vtkMapper handles vector
                );
              ConfigureColorLUT(coneLUT, 0.0, 1.0, // LUT scalar range
                0.0, 0.6,                          // LUT hue    range
                VTK_SCALE_LINEAR,                  // scale mode
                colorLutVectorMode,                // mode to handle vector
                1                                  // vector component Id
                );
              ConfigureProperty(coneProperty,
                1,                                   // shading
                propertyInterpolationMode,           // interpolation mode
                VTK_SURFACE,                         // VTK_POINTS / VTK_WIREFRAME
                0.6 + 0.4 * propertyColor, 0.6, 0.6, // RGB color
                0.2, 0.6, 0.3,                       // ambient, diffuse, specular
                120                                  // specular power
                );

              ConfigureMapper(cylinderMapper, -1.0, 1.0, // scalar range
                mapperScalarVisibility,                  // scalar visibility
                mapperScalarMode,                        // type of array provided
                VTK_COLOR_MODE_MAP_SCALARS,              // MapScalars
                0,                                       // use LUT scalar range
                0,                                       // interp before mapping
                "Normals",                               // color by array name
                0,                                       // array component index
                0                                        // vtkMapper handles vector
                );
              ConfigureColorLUT(cylinderLUT, 0.0, 1.0, // LUT scalar range
                0.0, 0.6,                              // LUT hue    range
                VTK_SCALE_LINEAR,                      // scale mode
                colorLutVectorMode,                    // mode to handle vector
                1                                      // vector component Id
                );
              ConfigureProperty(cylinderProperty,
                1,                                   // shading
                propertyInterpolationMode,           // interpolation mode
                VTK_SURFACE,                         // VTK_POINTS / VTK_WIREFRAME
                0.6 + 0.4 * propertyColor, 0.6, 0.6, // RGB color
                0.2, 0.6, 0.3,                       // ambient, diffuse, specular
                120                                  // specular power
                );
#endif

              renWin->Render();

              index++;
              if (index == 6)
              {
                retVal = vtkRegressionTestImage(renWin);
              }

              usleep(SLEEP_SECONDS * 1000000); // 1000000 is platform-dependent
            }
  }

// ---------------
// memory clean-up
#ifdef _MULTI_OBJECTS_
  cone->Delete();
  coneLUT->Delete();
  coneNormals->Delete();
  coneMapper->Delete();
  coneProperty->Delete();
  coneActor->Delete();

  cylinder->Delete();
  cylinderLUT->Delete();
  cylinderNormals->Delete();
  cylinderMapper->Delete();
  cylinderProperty->Delete();
  cylinderActor->Delete();
#endif

  sphere->Delete();
  sphereLUT->Delete();
  sphereNormals->Delete();
  sphereMapper->Delete();
  sphereProperty->Delete();
  sphereActor->Delete();

  camera->Delete();

  renderer->Delete();
  renWin->Delete();
  iren->Delete();

  if (!retVal)
  {
    // work around manta return code hijacking
    exit(-1);
  }
  return 0;
}

//----------------------------------------------------------------------------
void ConfigureMapper(vtkPolyDataMapper* mapper, double scalarMin, double scalarMax,
  int scalarVisibility, int scalarMode, int colorMode, int useLookupTableScalarRange,
  int interpolateScalarsBeforeMapping, const char* colorByArrayName,
  int colorByArrayVectorComponentIndex, int vectorByVTKmapper)
{
  // Set the scalar data range (min, max) that is then mapped into the
  // color LUT. The lack of this call might incur scalar data clamping
  // against the range of the color LUT if the two ranges are different.
  //
  // ONLY effective upon ( UseLookupTableScalarRange == OFF )
  //
  mapper->SetScalarRange(scalarMin, scalarMax);

  // specifiy whether or not a scalar data is used for coloring
  // the scalar data might be extracted from a given vector array
  //
  // mapper->ScalarVisibilityOn();
  // mapper->ScalarVisibilityOff();
  //
  mapper->SetScalarVisibility(scalarVisibility);

  // specify which KIND of data is used for color mapping
  //
  // Default           = 0  /  UsePointData     = 1  /  UseCellData  = 2
  // UsePointFieldData = 3  /  UseCellFieldData = 4  /  UseFieldData = 5
  //
  // mapper->SetScalarModeToDefault();
  // mapper->SetScalarModeToUsePointData();
  // mapper->SetScalarModeToUseCellData();
  // mapper->SetScalarModeToUsePointFieldData();
  // mapper->SetScalarModeToUseCellFieldData();
  // mapper->SetScalarModeToUseFieldData();
  //
  // VTK_SCALAR_MODE_DEFAULT              0
  // VTK_SCALAR_MODE_USE_POINT_DATA       1
  // VTK_SCALAR_MODE_USE_CELL_DATA        2
  // VTK_SCALAR_MODE_USE_POINT_FIELD_DATA 3
  // VTK_SCALAR_MODE_USE_CELL_FIELD_DATA  4
  // VTK_SCALAR_MODE_USE_FIELD_DATA       5
  //
  mapper->SetScalarMode(scalarMode);

  // allow the current color, specified by glColor(), to be efficiently
  // assigned to the material color(s) by means of glColorMaterial(), in
  // place of costly calls to glMaterial(...)
  //
  // Default = 0 / Ambient = 1 / Diffuse = 2 / AmbientAndDiffuse = 3
  //
  // mapper->SetScalarMaterialModeToDefault()
  // mapper->SetScalarMaterialModeToAmbient();
  // mapper->SetScalarMaterialModeToDiffuse();
  // mapper->SetScalarMaterialModeToAmbientAndDiffuse();
  //
  // VTK_MATERIALMODE_DEFAULT              0
  // VTK_MATERIALMODE_AMBIENT              1
  // VTK_MATERIALMODE_DIFFUSE              2
  // VTK_MATERIALMODE_AMBIENT_AND_DIFFUSE  3
  //
  // mapper->SetScalarMaterialMode( index );

  // specify whether unsigned char scalars, if any, are directly used as
  // colors or scalars need to be mapped to colors via an LUT
  //
  // Default = 0  /  MapScalars = 1
  //
  // mapper->SetColorModeToDefault();
  // mapper->SetColorModeToMapScalars();
  //
  // VTK_COLOR_MODE_DEFAULT     0
  // VTK_COLOR_MODE_MAP_SCALARS 1
  //
  mapper->SetColorMode(colorMode);

  // Specify whether the color LUT or the scalar data provides the actual
  // data range used for color mapping. The range of a default color LUT
  // is [ 0.0, 1.0 ]
  //
  // default = OFF
  // mapper->UseLookupTableScalarRangeOn();
  // mapper->UseLookupTableScalarRangeOff();
  //
  mapper->SetUseLookupTableScalarRange(useLookupTableScalarRange);

  // ONLY for UsePointFieldData, but NOT for UseCellFieldData
  //
  // default = OFF
  // mapper->InterpolateScalarsBeforeMappingOn();
  // mapper->InterpolateScalarsBeforeMappingOff();
  //
  mapper->SetInterpolateScalarsBeforeMapping(interpolateScalarsBeforeMapping);

  if (vectorByVTKmapper)
  {
    // Specify a vector array for color mapping and choose a certain
    // component as the active scalar data for the actual color map.
    //
    // achieves the same effect as obtained by the combination below
    //
    // vtkMapper::SelectColorArray( vectorArrayName )
    // vtkLookupTable::SetVectorModeToComponent();
    // vtkLookupTable::SetVectorComponent( vectorComponentIndex );
    //
    mapper->ColorByArrayComponent(colorByArrayName, colorByArrayVectorComponentIndex);
  }
  else
  {
    // ONLY upon ( scalar mode == UsePointFieldData  OR   UseCellFieldData )
    // and the color LUT converts vectors to colors based on the 'vector mode'
    //
    // coupled with vtkLookupTable::SetVectorModeToComponent()
    //         and  vtkLookupTable::SetVectorComponent( vectorComponentIndex )
    //
    mapper->SelectColorArray(colorByArrayName);
  }
}

//----------------------------------------------------------------------------
void ConfigureColorLUT(vtkLookupTable* clrLUT, double lutScalarMin, double lutScalarMax,
  double lutHueMin, double lutHueMax, int scaleMode, int vectorMode, int vectorComponentIndex)
{
  // the following two lines result in the same effect, i.e., to set the range
  // of the color LUT in contrast with the default one [ 0.0, 1.0 ]
  //
  // only effective upon vtkMapper:UseLookupTableScalarRangeOn()
  //
  clrLUT->SetRange(lutScalarMin, lutScalarMax);
  clrLUT->SetTableRange(lutScalarMin, lutScalarMax);

  // a range of hue for actual color mapping
  // default range = [ 0.0, 1.0 ]
  //
  clrLUT->SetHueRange(lutHueMin, lutHueMax);

  // scale mode that maps the input scalar data to the LUT scalar range
  //
  // clrLUT->SetScaleToLinear();
  // clrLUT->SetScaleToLog10();
  //
  // VTK_SCALE_LINEAR 0
  // VTK_SCALE_LOG10  1
  //
  clrLUT->SetScale(scaleMode);

  // the following two lines, coupled with vtkMapper::SelectColorArray
  // ( vectorArrayName ), achieve the same effect as obtained by
  // vtkMapper::ColorByArrayComponent ( vectorArrayName, componentIndex )
  // ====== UPON ( vectorMode == vtkScalarsToColors::COMPONENT ) ======
  //
  // clrLUT->SetVectorModeToMagnitude();
  // clrLUT->SetVectorModeToComponent();
  //
  // vtkScalarsToColors::COMPONENT / vtkScalarsToColors::MAGNITUDE
  //
  // clrLUT->SetVectorMode( vtkScalarsToColors::COMPONENT );
  // clrLUT->SetVectorMode( vtkScalarsToColors::MAGNITUDE );
  //
  clrLUT->SetVectorMode(vectorMode);
  clrLUT->SetVectorComponent(vectorComponentIndex);
}

//----------------------------------------------------------------------------
void ConfigureProperty(vtkProperty* property, int shading, int interpolationMode,
  int representationMode, double color_r, double color_g, double color_b, double ambientCoef,
  double diffuseCoef, double specularCoef, double specularPower)
{
  // Upon Shading ON, material must be provided
  //
  // property->ShadingOn();
  // property->ShadingOff();
  //
  property->SetShading(shading);

  // polygon-wise interpolation mode
  //
  // ToFlat = 0 / ToGouraud = 1 / ToPhong = 2
  //
  // property->SetInterpolationToFlat();
  // property->SetInterpolationToGouraud();
  // property->SetInterpolationToPhong();
  //
  // VTK_FLAT    0
  // VTK_GOURAUD 1
  // VTK_PHONG   2
  //
  property->SetInterpolation(interpolationMode);

  // representation mode
  //
  // ToPoints = 0 / ToWireframe = 1 / ToSurface = 2 (default)
  //
  // property->SetRepresentationToPoints();
  // property->SetRepresentationToWireframe();
  // property->SetRepresentationToSurface();
  //
  // VTK_POINTS    0
  // VTK_WIREFRAME 1
  // VTK_SURFACE   2
  //
  property->SetRepresentation(representationMode);

  if (representationMode == 3) // VTK_SURFACE | VTK_WIREFRAME
  {
    // wireframe does not mean Edge Visibility ON
    //
    // property->EdgeVisibilityOn();
    // property->EdgeVisibilityOff();
    //
    property->SetEdgeVisibility(1);
    property->SetRepresentationToSurface();
  }
  else
  {
    property->SetEdgeVisibility(0);
  }

  // set object color, and by default, ambient, diffuse, and specular colors
  property->SetColor(color_r, color_g, color_b);
  property->SetAmbient(ambientCoef);
  property->SetDiffuse(diffuseCoef);
  property->SetSpecular(specularCoef);
  property->SetSpecularPower(specularPower);
}

//----------------------------------------------------------------------------
void DumpAttributeInfo(vtkPolyData* polyData, char* objName)
{
  int i;
  vtkCellData* cellData = polyData->GetCellData();
  vtkPointData* pointData = polyData->GetPointData();

  if (cellData)
  {
    cerr << endl
         << objName << " cell  data has " << cellData->GetNumberOfArrays() << " array(s)" << endl;

    for (i = 0; i < cellData->GetNumberOfArrays(); i++)
    {
      cerr << "  array #" << i << " = " << cellData->GetArrayName(i) << endl;
    }
  }
  if (pointData)
  {
    cerr << endl
         << objName << " point data has " << pointData->GetNumberOfArrays() << " array(s)" << endl;
    for (i = 0; i < pointData->GetNumberOfArrays(); i++)
    {
      cerr << "  array #" << i << " = " << pointData->GetArrayName(i) << endl;
    }
  }

  cellData = NULL;
  pointData = NULL;
}

//----------------------------------------------------------------------------
void DumpColorMapInfo(vtkPolyDataMapper* mapper, vtkProperty* property, int vectorByVTKmapper)
{
  vtkLookupTable* lut = vtkLookupTable::SafeDownCast(mapper->GetLookupTable());

  cerr << endl
       << "Mapper Information" << endl
       << "  Scalar Visibility    = " << mapper->GetScalarVisibility()
       << " (whether or not scalar data is used for coloring)" << endl
       << "  Scalar Mode          = " << mapper->GetScalarMode()
       << " ( Default = 0;  PointData = 1;  CellData = 2; "
       << " PointFieldData = 3;  CellFieldData = 4;  FieldData = 5 )" << endl
       << "  Scalar Data Range    = (" << mapper->GetScalarRange()[0] << ", "
       << mapper->GetScalarRange()[1] << ")" << endl
       << "  Array Name           = " << mapper->GetArrayName() << endl
       << "  Array Component      = " << mapper->GetArrayComponent()
       << " (only upon vector array used for color mapping)" << endl
       << "  vector by vtkMapper  = " << vectorByVTKmapper
       << " (vector array handled by vtkMapper or vtkLookupTable)" << endl
       << "  Scalar Material Mode = " << mapper->GetScalarMaterialModeAsString()
       << " ( Default / Ambient / Diffuse / AmbientAndDiffuse )" << endl
       << "  Color  Mode          = " << mapper->GetColorModeAsString()
       << " ( Default <unsigned char scalars directly used as colors> /"
       << " MapScalars <through color LUT> )" << endl
       << endl
       << "  Use Lookup Table Scalar Range   = " << mapper->GetUseLookupTableScalarRange() << endl
       << "  InterpolateScalarsBeforeMapping = " << mapper->GetInterpolateScalarsBeforeMapping()
       << endl
       << "  Geometric Data Bounding Box     = " << mapper->GetBounds()[0]
       << " <= x <= " << mapper->GetBounds()[1] << ";  " << mapper->GetBounds()[2]
       << " <= y <= " << mapper->GetBounds()[3] << ";  " << mapper->GetBounds()[4]
       << " <= z <= " << mapper->GetBounds()[5] << endl

       << endl
       << "Lookup Table Information (accessed through mapper)" << endl
       << "  LUT scalar Range = (" << lut->GetRange()[0] << ", " << lut->GetRange()[1] << ")"
       << " --- possibly different from the scalar DATA range" << endl
       << "  Table Range      = (" << lut->GetTableRange()[0] << ", " << lut->GetTableRange()[1]
       << ")"
       << " --- exactly the same as LUT Scalar Range" << endl
       << "  Hue    Range     = (" << lut->GetHueRange()[0] << ", " << lut->GetHueRange()[1] << ")"
       << endl
       << "  Alpha Value      = " << lut->GetAlpha() << endl
       << "  Alpha Range      = (" << lut->GetAlphaRange()[0] << ", " << lut->GetAlphaRange()[1]
       << ")" << endl
       << "  Value Range      = (" << lut->GetValueRange()[0] << ", " << lut->GetValueRange()[1]
       << ")" << endl
       << "  Number of Colors = " << lut->GetNumberOfColors() << endl
       << "  Scale Mode       = " << lut->GetScale()
       << " ( VTK_SCALE_LINEAR = 0 / VTK_SCALE_LOG10 = 1 )" << endl
       << "  Vector Mode      = " << lut->GetVectorMode()
       << " ( Magnitude = 0;  Component = 1 <default> )" << endl
       << "  Vector Component = " << lut->GetVectorComponent()
       << " (only upon vector array used for color mapping)" << endl

       << endl
       << "Property Information" << endl
       << "  Shading             = " << property->GetShading() << endl
       << "  RGB Color           = (" << property->GetColor()[0] << ", " << property->GetColor()[1]
       << ", " << property->GetColor()[2] << ")" << endl
       << "  Ambient Coefficient = " << property->GetAmbient() << endl
       << "  Diffuse Coefficient = " << property->GetDiffuse() << endl
       << "  Specular Coeficient = " << property->GetSpecular() << endl
       << "  Specular Power      = " << property->GetSpecularPower()
       //<< endl << "  Material Name       = "  << property->GetMaterialName()
       << endl
       << "  Edge Visibility     = " << property->GetEdgeVisibility()
       << " (Wireframe does not implicitly set Edge Visibility on)" << endl
       << "  Interpolation Mode  = " << property->GetInterpolationAsString()
       << " ( Flat / Gouraud / Phong )" << endl
       << "  Representation Mode = " << property->GetRepresentationAsString()
       << " ( Points / Wireframe / Surface )" << endl
       << endl;

  lut = NULL;
}
