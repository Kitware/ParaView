#!/bin/bash


    #
cmake \
    -DCMAKE_C_COMPILER=/opt/apps/intel/11.1/bin/intel64/icc \
    -DCMAKE_CXX_COMPILER=/opt/apps/intel/11.1/bin/intel64/icpc \
    -DCMAKE_LINKER=/opt/apps/intel/11.1/bin/intel64/icpc \
    -DCMAKE_CXX_FLAGS=-Wno-deprecated \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTING=OFF \
    -DPARAVIEW_BUILD_QT_GUI=OFF \
    -DPARAVIEW_USE_MPI=ON \
    -DMPI_COMPILER=/opt/apps/intel11_1/openmpi/1.3.3/bin/mpicxx \
    -DMPI_EXTRA_LIBRARY=/opt/apps/intel11_1/openmpi/1.3.3/lib/libmpi.so\;/opt/apps/intel11_1/openmpi/1.3.3/lib/libopen-rte.so\;/opt/apps/intel11_1/openmpi/1.3.3/lib/libopen-pal.so\;/usr/lib64/libdl.so\;/usr/lib64/libnsl.so\;/usr/lib64/libutil.so \
    -DMPI_INCLUDE_PATH=/opt/apps/intel11_1/openmpi/1.3.3/include \
    -DMPI_LIBRARY=/opt/apps/intel11_1/openmpi/1.3.3/lib/libmpi.so \
    -DMPI_LINK_FLAGS=-Wl,--export-dynamic \
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
    -DPARAVIEW_BUILD_PLUGIN_PointSprite=ON \
    -DPARAVIEW_BUILD_PLUGIN_SierraPlotTools=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SLACTools=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SQLDatabaseGraphSourcePanel=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SQLDatabaseTableSourcePanel=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SplitTableFieldPanel=OFF \
    -DPARAVIEW_BUILD_PLUGIN_StatisticsToolbar=OFF \
    -DPARAVIEW_BUILD_PLUGIN_SurfaceLIC=ON \
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
