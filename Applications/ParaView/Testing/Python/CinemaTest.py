import os, json

from paraview import simple
from paraview import smtesting
from paraview import data_exploration as cinema

smtesting.ProcessCommandLineArguments()
work_directory = os.path.join(smtesting.TempDir, 'cinema')
has_rgbz_view = True

try:
    simple.LoadDistributedPlugin('RGBZView', ns=globals())
except:
    has_rgbz_view = False

# === Data to explore and configuration =======================================

data_to_explore = simple.Wavelet()
min = 63.96153259277344
max = 250.22056579589844
center_of_rotation = [0.0, 0.0, 0.0]
rotation_axis = [0.0, 0.0, 1.0]
angle_steps = (72, 60)
distance = 60
lut = simple.GetLookupTableForArray(
    "RTData", 1,
    RGBPoints=[min, 0.23, 0.299, 0.754, (min+max)*0.5, 0.865, 0.865, 0.865, max, 0.706, 0.016, 0.15],
    ColorSpace='Diverging',
    ScalarRangeInitialized=1.0 )
iso_values = [ ((float(x) * (max-min) * 0.1) + min) for x in range(10)]

# === Create analysis =========================================================

analysis = cinema.AnalysisManager( work_directory, "Cinema Test", "Test various cinema explorers.")
analysis.begin()

# === SliceExplorer ===========================================================

analysis.register_analysis(
    "slice",                            # id
    "Slice exploration",                # title
    "Perform 10 slice along X",         # description
    "{sliceColor}_{slicePosition}.jpg", # data structure
    cinema.SliceExplorer.get_data_type())
nb_slices = 5
colorByArray = { "RTData": { "lut": lut , "type": 'POINT_DATA'} }
view = simple.CreateRenderView()

fng = analysis.get_file_name_generator("slice")
exporter = cinema.SliceExplorer(fng, view, data_to_explore, colorByArray, nb_slices)
exporter.set_analysis(analysis)

# Explore
exporter.UpdatePipeline()

# === ContourExplorer + ThreeSixtyImageStackExporter ==========================

analysis.register_analysis(
    "contour-360",                                  # id
    "Contour",                                      # title
    "Perform 15 contour",                           # description
    "{contourBy}/{contourValue}/{theta}_{phi}.jpg", # data structure
    cinema.ThreeSixtyImageStackExporter.get_data_type())
fng = analysis.get_file_name_generator("contour-360")
arrayName = ('POINT_DATA', 'RTData')
view2 = simple.CreateRenderView()

cExplorer = cinema.ContourExplorer(fng, data_to_explore, arrayName, [min,max], 15)
proxy = cExplorer.getContour()
rep = simple.Show(proxy, view2)
rep.LookupTable = lut
rep.ColorArrayName = arrayName

exp = cinema.ThreeSixtyImageStackExporter(fng, view2, center_of_rotation, distance, rotation_axis, angle_steps)
for progress in cExplorer:
    exp.UpdatePipeline()

# === ImageResampler ==========================================================

analysis.register_analysis(
    "interactive-prober",                          # id
    "Interactive prober",                          # title
    "Sample data in image stack for line probing", # description
    "{field}/{slice}.{format}",                    # data structure
    cinema.ImageResampler.get_data_type())
fng = analysis.get_file_name_generator("interactive-prober")
arrays = { "RTData" : lut }
exp = cinema.ImageResampler(fng, data_to_explore, [21,21,21], arrays)
exp.UpdatePipeline()



# === CompositeImageExporter ==================================================
# CompositeImageExporter(file_name_generator, data_list, colorBy_list, luts, camera_handler, view_size, data_list_pipeline, axisVisibility=1, orientationVisibility=1, format='jpg')
if has_rgbz_view:
    analysis.register_analysis(
        "composite",
        "Composite rendering",
        "Performing composite on contour",
        '{theta}/{phi}/{filename}', cinema.CompositeImageExporter.get_data_type())
    fng = analysis.get_file_name_generator("composite")

    # Create pipeline to compose
    contour_values = [ 64.0, 90.6, 117.2, 143.8, 170.4, 197.0, 223.6, 250.2]
    color_type = [('POINT_DATA', "RTData")]
    luts = { "RTData": lut }
    filters = [ data_to_explore ]
    filters_description = [ {'name': 'Wavelet'} ]
    color_by = [ color_type ]

    for iso_value in contour_values:
        filters.append( simple.Contour( Input=data_to_explore, PointMergeMethod="Uniform Binning", ContourBy = ['POINTS', 'RTData'], Isosurfaces = [iso_value], ComputeScalars = 1 ) )
        color_by.append( color_type )
        filters_description.append({'name': 'iso=%s' % str(iso_value)})


    # Data exploration ------------------------------------------------------------
    camera_handler = cinema.ThreeSixtyCameraHandler(fng, None, [ float(r) for r in range(0, 360, 72)], [ float(r) for r in range(-60, 61, 45)], center_of_rotation, rotation_axis, distance)
    exporter = cinema.CompositeImageExporter(fng, filters, color_by, luts, camera_handler, [400,400], filters_description, 0, 0)
    exporter.set_analysis(analysis)
    exporter.UpdatePipeline()

# === TODO ====================================================================
# ThreeSixtyImageStackExporter(file_name_generator, view_proxy, focal_point=[0.0,0.0,0.0], distance=100.0, rotation_axis=[0,0,1], angular_steps=[10,15])
# LineProber(file_name_generator, data_to_probe, points_series, number_of_points)
# DataProber(file_name_generator, data_to_probe, points_series, fields)
# TimeSerieDataProber(file_name_generator, data_to_probe, point_series, fields, time_to_write)
# === TODO ====================================================================

analysis.end()

# Print analysis info.json
analysisInfo = None
with open(os.path.join(work_directory, 'info.json'), 'r') as json_file:
    analysisInfo = json.load(json_file)

print json.dumps(analysisInfo, sort_keys=True, indent=4, separators=(',', ': '))

# === Execute embedded test functions =========================================

test_dir = os.path.join(smtesting.TempDir, 'cinema-test')
cinema.test(test_dir)
cinema.test2(test_dir)
cinema.test3(test_dir)