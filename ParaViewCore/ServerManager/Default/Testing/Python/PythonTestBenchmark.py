from paraview.simple import *

import paraview.benchmark as pvb
pvb.maximize_logs()

w = Wavelet()
w.UpdatePipeline()

pvb.get_logs()

pvb.print_logs()

pvb.dump_logs("benchmark.log")

pvb.import_logs("benchmark.log")

pvb.print_logs()

print 'SUCCESS'
