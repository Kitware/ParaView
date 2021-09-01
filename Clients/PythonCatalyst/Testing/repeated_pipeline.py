import paraview
from paraview import print_info

if hasattr(paraview, "repeated_pipeline_count"):
    paraview.repeated_pipeline_count += 1
else:
    setattr(paraview, "repeated_pipeline_count", 1)

# If you change the txt here, don't forget to update the CMakeLists.txt
# for `PASS_REGULAR_EXPRESSION`.
print_info("%s: Importing 'repeated_pipeline' for %d-th time!" % (__name__, paraview.repeated_pipeline_count))
