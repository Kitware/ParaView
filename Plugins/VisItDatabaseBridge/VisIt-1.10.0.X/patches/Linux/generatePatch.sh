# VisIt-Patches
#===============
# 
# for f in `find ./ -type f -name 'Makefile.in'`; do diff -u ../../../visit1.10.0/src/$f $f; done > VisItPV3Build.in.patch
# 
# 
# vtkIdType issues on x86_64:
# I gave up on this and went with VTK_USE_64_IDS OFF
# 
# avt/Pipeline/Data/avtCommonDataFunctions.C
# avt/Filters/avtFeatureEdgesFilter.C
# avt/Filters/avtCoordSystemConvert.C
# avt/Queries/Pick/avtLocateCellQuery.C
# avt/MIR/Zoo/ZooMIR.C
# databases/ANSYS/avtANSYSFileFormat.C
# databases/Dyna3D/avtDyna3DFileFormat.C
# databases/ExtrudedVol/avtExtrudedVolFileFormat.C
# databases/NASTRAN/avtNASTRANFileFormat.C
# databases/PATRAN/avtPATRANFileFormat.C
# databases/Silo/avtSiloFileFormat.C
# databases/TSurf/avtTSurfFileFormat.C
# databases/Tecplot/avtTecplotFileFormat.C
# databases/Vista/avtVistaAle3dFileFormat.C
# databases/Vista/avtVistaDiabloFileFormat.C
# databases/UNIC/avtUNICFileFormat.C
# databases/XDMF/avtXDMFFileFormat.C
# databases/KullLite/avtKullLiteFileFormat.C
# databases/SimV1/avtSimV1FileFormat.C
# visit_vtk/lightweight/vtkPolyDataRelevantPointsFilter.C
# visit_vtk/lightweight/vtkUnstructuredGridFacelistFilter.C