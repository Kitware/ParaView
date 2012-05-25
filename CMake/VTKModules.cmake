#########################################################################
# This file turns on the modules needed by ParaView
include (vtkDependentOption)

# First turn of stand alone group 
set(VTK_Group_StandAlone OFF CACHE BOOL "" FORCE)
mark_as_advanced(VTK_Group_StandAlone)
set(VTK_Group_Rendering OFF CACHE BOOL "" FORCE)
mark_as_advanced(VTK_Group_Rendering)

# The VTK modules needed when MPI is turned on
set(_vtk_mpi_modules
  vtkParallelMPI
  vtkFiltersParallelImaging
  vtkIOMPIImage
  vtkFiltersParallelMPI
  vtkFiltersParallelTracers
  # Note: Not in ParaViewXXX.xml but required by a test.
  # Needed for:
  #  vtkPStreamTracer
  
  vtkIOParallelNetCDF
  # Needed for:
  #  vtkPNetCDFPOPReader
  )

# Turn on based on the value of PARAVIEW_USE_MPI  
foreach(_mod ${_vtk_mpi_modules})
  set(_full_name Module_${_mod})
  set(${_full_name} ${PARAVIEW_USE_MPI} CACHE BOOL "" FORCE)
  mark_as_advanced(${_full_name})
endforeach()

# Turn Cosmo and VPIC MPI build flags based on value of PARAVIEW_USE_MPI
set(VTK_COSMO_USE_MPI ${PARAVIEW_USE_MPI} CACHE BOOL "" FORCE)
mark_as_advanced(VTK_COSMOS_USE_MPI)
set(VTK_VPIC_USE_MPI ${PARAVIEW_USE_MPI} CACHE BOOL "" FORCE)
mark_as_advanced(VTK_VPIC_USE_MPI)


set(_vtk_modules
  # VTK modules which ParaView has a explicity compile
  # time dependency on
  vtkRenderingVolume
  vtkRenderingLabel
  vtkRenderingFreeType
  vtkRenderingFreeTypeOpenGL
  vtkRenderingVolumeOpenGL
  vtkRenderingOpenGL
  vtkRenderingLOD
  vtkRenderingHybridOpenGL
  vtkRenderingContext2D
  vtkRenderingAnnotation
  vtkInteractionStyle
  vtkInteractionWidgets
  vtkRenderingParallel
  vtkFiltersParallel
  vtkIOParallelExodus
  vtkFiltersParallelStatistics
  vtkIOParallel
  vtkFiltersGeneric
  vtkFiltersHyperTree
  vtkImagingFourier
  vtkImagingMorphological
  vtkIOImport
  vtkIOXML
  vtkIOImage
  vtkIOGeometry
  vtklibxml2
  vtkViewsContext2D
  vtkIOExport
  vtkIOInfovis
  vtkFiltersAMR
  vtkChartsCore
  vtkIOEnSight
  vtkTestingRendering
  # Needed to for tests that are built even with BUILD_TESTING off!
  vtkInteractionImage
  if(PARAVIEW_USE_VISITBRIDGE)
    # Needed by VisItBridge
    vtkFiltersTracers
  endif() 
  # Modules that are required a runtime generated from:
  #
  # ParaViewFilters.xml
  # ParaViewReaders.xml
  # ParaViewSources.xml
  # ParaViewWriters.xml
  # 
  # Note: Some are duplicates of the above, but they are listed to record the
  # implicit dependancy
  vtkAMRCore
  # Needed for:
  #  vtkAMRResampleFilter
  #  vtkAMRCutPlane
  #  vtkAMRGaussianPulseSource

  vtkCommonExecutionModel
  # Needed for:
  #  vtkUniformGridPartitioner

  vtkFiltersCore
  # Needed for:
  #  vtkThreshold
  #  vtkAppendPolyData
  #  vtkCellDataToPointData
  #  vtkCleanPolyData
  #  vtkDecimatePro
  #  vtkDelaunay2D
  #  vtkDelaunay3D
  #  vtkElevationFilter
  #  vtkFeatureEdges
  #  vtkMaskPoints
  #  vtkPointDataToCellData
  #  vtkPolyDataNormals
  #  vtkQuadricClustering
  #  vtkSmoothPolyDataFilter
  #  vtkStripper
  #  vtkStructuredGridOutlineFilter
  #  vtkThreshold
  #  vtkTriangleFilter
  #  vtkTubeFilter
  #  vtkDataObjectGenerator

  vtkFiltersExtraction
  # Needed for:
  #  vtkExtractGrid
  #  vtkExtractSelection
  #  vtkExtractBlock
  #  vtkExtractEdges
  #  vtkExtractGeometry
  #  vtkExtractGrid
  #  vtkExtractSelection

  vtkFiltersGeneral
  # Needed for:
  #  vtkWarpVector
  #  vtkTemporalStatistics
  #  vtkBlockIdScalars
  #  vtkBrownianPoints
  #  vtkCellCenters
  #  vtkCellDerivatives
  #  vtkClipClosedSurface
  #  vtkCurvatures
  #  vtkDataSetTriangleFilter
  #  vtkImageDataToPointSet
  #  vtkLevelIdScalars
  #  vtkQuadraturePointInterpolator
  #  vtkQuadraturePointsGenerator
  #  vtkQuadratureSchemeDictionaryGenerator
  #  vtkRectilinearGridToPointSet
  #  vtkReflectionFilter
  #  vtkShrinkFilter
  #  vtkTemporalStatistics
  #  vtkTessellatorFilter
  #  vtkTransformFilter
  #  vtkWarpScalar
  #  vtkWarpVector
  #  vtkYoungsMaterialInterface
  #  vtkTableToPolyData
  #  vtkTableToStructuredGrid
  #  vtkAxes

  vtkFiltersGeneric
  # Needed for:
  #  vtkGenericClip
  #  vtkGenericStreamTracer
  #  vtkGenericGeometryFilter

  vtkFiltersGeometry
  # Needed for:
  #  vtkDataSetSurfaceFilter

  vtkFiltersHybrid
  # Needed for:
  #  vtkFacetReader
  #  vtkTemporalInterpolator
  #  vtkTemporalSnapToTimeStep
  #  vtkTemporalShiftScale
  #  vtkTemporalInterpolator
  #  vtkTemporalSnapToTimeStep
  #  vtkTemporalShiftScale

  vtkFiltersModeling
  # Needed for:
  #  vtkLinearExtrusionFilter
  #  vtkLoopSubdivisionFilter
  #  vtkOutlineFilter
  #  vtkRibbonFilter
  #  vtkRotationalExtrusionFilter

  vtkFiltersParallel
  # Needed for:
  #  vtkProcessIdScalars

  vtkFiltersProgrammable
  # Needed for:
  #  vtkProgrammableFilter
  #  vtkProgrammableFilter

  vtkFiltersSources
  # Needed for:
  #  vtkOutlineCornerFilter
  #  vtkGlyphSource2D
  #  vtkArrowSource
  #  vtkCubeSource
  #  vtkConeSource
  #  vtkCylinderSource
  #  vtkDiskSource
  #  vtkLineSource
  #  vtkOutlineSource
  #  vtkPlaneSource
  #  vtkPointSource
  #  vtkProgrammableSource
  #  vtkSphereSource
  #  vtkSuperquadricSource
  #  vtkTextSource

  vtkFiltersStatistics
  # Needed for:
  #  vtkContingencyStatistics
  #  vtkDescriptiveStatistics
  #  vtkMultiCorrelativeStatistics
  #  vtkPCAStatistics

  vtkFiltersTexture
  # Needed for:
  #  vtkTextureMapToCylinder
  #  vtkTextureMapToPlane
  #  vtkTextureMapToSphere

  #vtkFiltersTracers
  # Needed for:
  #  vtkStreamTracer
  #  vtkStreamTracer

  vtkFiltersVerdict
  # Needed for:
  #  vtkMeshQuality

  vtkImagingCore
  # Needed for:
  #  vtkRTAnalyticSource

  vtkImagingHybrid
  # Needed for:
  #  vtkGaussianSplatter

  vtkImagingSources
  # Needed for:
  #  vtkImageMandelbrotSource

  vtkIOExodus
  # Needed for:
  #  vtkExodusIIReader

  vtkIOGeometry
  # Needed for:
  #  vtkTecplotReader
  #  vtkBYUReader
  #  vtkOBJReader
  #  vtkProStarReader
  #  vtkPDBReader
  #  vtkSTLReader
  #  vtkSESAMEReader
  #  vtkMFIXReader
  #  vtkFLUENTReader
  #  vtkOpenFOAMReader
  #  vtkParticleReader
  #  vtkDataSetWriter

  vtkIOImage
  # Needed for:
  #  vtkDEMReader
  #  vtkGaussianCubeReader
  #  vtkImageReader
  #  vtkMetaImageReader
  #  vtkMetaImageWriter
  #  vtkPNGWriter
  #  vtkNrrdReader

  vtkIONetCDF
  # Needed for:
  #  vtkNetCDFReader
  #  vtkSLACReader
  #  vtkSLACParticleReader
  #  vtkNetCDFCAMReader
  #  vtkNetCDFPOPReader
  #  vtkMPASReader

  vtkIOParallel
  # Needed for:
  #  vtkWindBladeReader
  #  vtkPNetCDFPOPReader
  #  vtkPDataSetWriter
  #  vtkExodusIIWriter
  #  vtkEnSightWriter

  vtkIOPLY
  # Needed for:
  #  vtkPLYReader

  vtkIOVPIC
  # Needed for:
  #  vtkVPICReader

  vtkIOXML
  # Needed for:
  #  vtkXMLPolyDataReader
  #  vtkXMLUnstructuredGridReader
  #  vtkXMLImageDataReader
  #  vtkXMLStructuredGridReader
  #  vtkXMLRectilinearGridReader
  #  vtkXMLPPolyDataReader
  #  vtkXMLPUnstructuredGridReader
  #  vtkXMLPImageDataReader
  #  vtkXMLPStructuredGridReader
  #  vtkXMLPRectilinearGridReader
  #  vtkXMLMultiBlockDataReader
  #  vtkXMLHierarchicalBoxDataReader
  #  vtkXMLHyperOctreeWriter
  #  vtkXMLPolyDataWriter
  #  vtkXMLUnstructuredGridWriter
  #  vtkXMLStructuredGridWriter
  #  vtkXMLRectilinearGridWriter
  #  vtkXMLImageDataWriter
  #  vtkXMLPPolyDataWriter
  #  vtkXMLPUnstructuredGridWriter
  #  vtkXMLPStructuredGridWriter
  #  vtkXMLPRectilinearGridWriter
  #  vtkXMLPImageDataWriter
  #  vtkXMLMultiBlockDataWriter
  #  vtkXMLHierarchicalBoxDataWriter

  vtkRenderingFreeType
  # Needed for:
  #  vtkVectorText
  
  vtkFiltersCosmo
  # Note: Not in ParaViewXXX.xml but required by a test.
  # Needed for:
  #  vtkPCosmoReader

  vtkIOParallelLSDyna
  # Note: Not in ParaViewXXX.xml but required by a test.
  # Needed for:
  #  vtkPLSDynaReader

  vtkDomainsChemistry
  # Needed for:
  #  vtkMoleculeRepresentation
  )

# Are we building the GUI

set (PARAVIEW_BUILD_QT_GUI_NOT TRUE)
if (PARAVIEW_BUILD_QT_GUI)
  set (PARAVIEW_BUILD_QT_GUI_NOT FALSE)
endif()

VTK_DEPENDENT_OPTION(PARAVIEW_ENABLE_QT_SUPPORT
  "Build ParaView with Qt support (without GUI)" OFF
  "PARAVIEW_BUILD_QT_GUI_NOT" ON)

set(Module_vtkGUISupportQt ${PARAVIEW_ENABLE_QT_SUPPORT} CACHE BOOL "" FORCE)
mark_as_advanced(Module_vtkGUISupportQt)
  
# Modules needed if testing is on
# Note: Again there may be duplicated this intended to record the dependancy
if(BUILD_TESTING)
  list(APPEND _vtk_modules 
    vtkFiltersProgrammable)
endif()

if(BUILD_EXAMPLES)
  list(APPEND _vtk_modules 
    vtkTestingCore)
endif()
  
# Now enable the modules
foreach(_mod ${_vtk_modules})
  set(_full_name Module_${_mod})
  set(${_full_name} ON CACHE BOOL "" FORCE)
  mark_as_advanced(${_full_name})
endforeach()
