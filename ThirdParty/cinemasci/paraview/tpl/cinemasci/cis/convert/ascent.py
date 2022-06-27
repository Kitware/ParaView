import h5py
import numpy
import glob
import cinemasci.cdb
import os.path

class ascent():

    def __init__(self):
        self.outcdb    = None
        self.basename  = None 
        self.dirname   = None 
        self.cycles    = []
        self.minmax    = {}
        self.variables = [] 


    #
    # convert an ascent hdf5 data set of float images to a cinema database
    #
    # this function looks in the "indir" directory for files matching the pattern:
    #    <something>.cycle_<0 padded number>
    #
    def convert(self, indir, outcdb):
        self.indir  = indir
        self.outcdb = outcdb

        # initialize the cinema database 
        cdb = cinemasci.new("cdb", {"path": self.outcdb})
        cdb.initialize()

        self.__get_cycles()
        self.__get_variable_ranges()

        curextract = 0
        data = {}
        for c in self.cycles:
            hfile = os.path.join(self.dirname, "{}.cycle_{}/domain_000000.hdf5".format(self.basename, c))
            with h5py.File(hfile, "r") as bpf:
                # shape
                w = bpf["coordsets/coords/dims/i"][0] - 1
                h = bpf["coordsets/coords/dims/j"][0] - 1
    
                for v in self.variables:
                    # write the compressed data
                    data[v] = bpf.get("fields/{}/values".format(v))[...].reshape((w,h))
                    numpy.savez_compressed("{}/{}".format(self.outcdb, str(curextract).zfill(6)), data=data[v])
    
                    channel = v
                    if channel == 'depth':
                        channel = 'CISDepth'
                    # insert an entry in to the database
                    id = cdb.add_entry({'cycle':                c, 
                                        'CISImage':             'cycle_{}'.format(c.zfill(6)), 
                                        'CISVersion':           '1.0', 
                                        'CISOrigin':            'UL', 
                                        'CISImageWidth':        w,
                                        'CISImageHeight':       h,
                                        'CISLayer':             'layer0',
                                        'CISLayerOffsetX':      0, 
                                        'CISLayerOffsetY':      0, 
                                        'CISLayerWidth':        w, 
                                        'CISLayerHeight':       h, 
                                        'CISChannel':           channel, 
                                        'CISChannelVar':        v, 
                                        'CISChannelVarType':    'float', 
                                        'CISChannelVarMin':     self.minmax[v][0], 
                                        'CISChannelVarMax':     self.minmax[v][1],
                                        'FILE':                 '{}.npz'.format(str(curextract).zfill(6))
                                       })
                    curextract = curextract + 1
    
        cdb.finalize()

    #
    # run through the data, computing the min/max of all variables
    #
    def __get_variable_ranges(self):

        for c in self.cycles:
            hfile = os.path.join(self.dirname, "{}.cycle_{}/domain_000000.hdf5".format(self.basename, c))
            with h5py.File(hfile, "r") as bpf:
                w = bpf["coordsets/coords/dims/i"][0] - 1
                h = bpf["coordsets/coords/dims/j"][0] - 1

                # get the variable names
                fields = bpf["fields"]
                for val in fields.keys():
                    if not val == "ascent_ghosts":
                        if not val in self.variables:
                            self.variables.append(val)

                # get the min/max variables
                for v in self.variables:
                    data = bpf.get("fields/{}/values".format(v))[...].reshape((w,h))
                    if v in self.minmax:
                        vmin = numpy.nanmin(data)
                        vmax = numpy.nanmax(data)
                        if vmin < self.minmax[v][0]:
                            self.minmax[v][0] = vmin
                        if vmax > self.minmax[v][1]:
                            self.minmax[v][1] = vmax
                    else:
                        self.minmax[v] = [numpy.nanmin(data), numpy.nanmax(data)]

    def __get_cycles(self):
        cycles = []
        hpaths = glob.glob("{}/*.cycle_[0-9]*.root".format(self.indir))

        self.dirname   = os.path.dirname(hpaths[0])
        self.basename  = os.path.basename(hpaths[0]).split('.')[0]

        for p in hpaths:
            split_path = p.split('_')
            cycle = split_path[-1].split('.')
            self.cycles.append(cycle[0])

        self.cycles.sort()
