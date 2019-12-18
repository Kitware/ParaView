from paraview.simple import *

import paraview.benchmark as pvb
pvb.logbase.maximize_logs()

w = Wavelet()
w.UpdatePipeline()

pvb.logbase.get_logs()
pvb.logbase.print_logs()
pvb.logbase.dump_logs("benchmark.log")
pvb.logbase.import_logs("benchmark.log")

print('='*40)
print('Raw logs:')
print('='*40)
pvb.logbase.print_logs()

print('='*40)
print('Parsed logs:')
print('='*40)
comp_rank_frame_logs = pvb.logparser.process_logs()
for c, rank_frame_logs in comp_rank_frame_logs.items():
    print('-'*40)
    print('Component: %s' % c)
    print('-'*40)
    for r in range(0,len(rank_frame_logs)):
        print('.'*40)
        print('    Rank: %s' % r)
        print('.'*40)
        for f in rank_frame_logs[r]:
            print(f)

print('SUCCESS')
