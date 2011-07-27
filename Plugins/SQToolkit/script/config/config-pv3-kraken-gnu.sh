#!/bin/bash

# Set the path to the mpi install and choose one use on of
# the fololowing:
#
MPI2=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-gnu
# MPI2=/nics/c/home/bloring/apps/mpich2-1.2.1p1-gnu
#
# PVMPI="-DMPI_COMPILER=$MPI2/bin/mpicc"
PVMPI="-DMPI_INCLUDE_PATH=$MPI2/include -DMPI_LIBRARY=$MPI2/lib/libmpich.a"

MESA=/nics/c/home/bloring/apps/Mesa-7.7-osmesa-gnu

cmake \
    -DCMAKE_C_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/cc \
    -DCMAKE_CXX_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/CC \
    -DCMAKE_LINKER=/opt/cray/xt-asyncpe/3.6/bin/CC \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DPARAVIEW_BUILD_QT_GUI=OFF \
    -DVTK_USE_X=OFF \
    -DVTK_OPENGL_HAS_OSMESA=ON \
    -DOPENGL_INCLUDE_DIR=$MESA/include \
    -DOPENGL_gl_LIBRARY="" \
    -DOPENGL_glu_LIBRARY=$MESA/lib/libGLU.a \
    -DOPENGL_xmesa_INCLUDE_DIR=$MESA/include \
    -DOSMESA_INCLUDE_DIR=$MESA/include \
    -DOSMESA_LIBRARY=$MESA/lib/libOSMesa.a \
    -DPARAVIEW_USE_MPI=ON \
    $PVMPI \
    -DPARAVIEW_BUILD_PLUGIN_Array=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ChartViewFrame=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ClientChartView=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ClientGeoView=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ClientGeoView2D=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ClientGraphView=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ClientRecordView=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ClientRichTextView=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ClientTableView=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ClientTreeView=OFF \
    -DPARAVIEW_BUILD_PLUGIN_CommonToolbar=OFF \
    -DPARAVIEW_BUILD_PLUGIN_CosmoFilters=OFF \
    -DPARAVIEW_BUILD_PLUGIN_GraphLayoutFilterPanel=OFF \
    -DPARAVIEW_BUILD_PLUGIN_Infovis=OFF \
    -DPARAVIEW_BUILD_PLUGIN_Manta=OFF \
    -DPARAVIEW_BUILD_PLUGIN_Moments=OFF \
    -DPARAVIEW_BUILD_PLUGIN_NetDMFReader=OFF \
    -DPARAVIEW_BUILD_PLUGIN_Prism=OFF \
    -DPARAVIEW_BUILD_PLUGIN_PointSprite=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SierraPlotTools=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SLACTools=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SQLDatabaseGraphSourcePanel=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SQLDatabaseTableSourcePanel=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SplitTableFieldPanel=OFF \
    -DPARAVIEW_BUILD_PLUGIN_StatisticsToolbar=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SurfaceLIC=OFF \
    -DPARAVIEW_BUILD_PLUGIN_TableToGraphPanel=OFF \
    -DPARAVIEW_BUILD_PLUGIN_TableToSparseArrayPanel=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ThresholdTablePanel=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ClientGraphViewFrame=OFF \
    -DPARAVIEW_BUILD_PLUGIN_ClientTreeAreaView=OFF \
    -DPARAVIEW_BUILD_PLUGIN_VisItDatabaseBridge=OFF \
    -DPARAVIEW_BUILD_PLUGIN_H5PartReader=OFF \
    -DPARAVIEW_BUILD_PLUGIN_CoProcessingScriptGenerator=OFF \
    -DPARAVIEW_BUILD_PLUGIN_AnalyzeNIfTIReaderWriter=OFF \
    -DPARAVIEW_BUILD_PLUGIN_VisTrails=OFF \
    $* 

