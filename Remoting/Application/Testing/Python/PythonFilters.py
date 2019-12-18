import os.path
from paraview.simple import *
import sys
from paraview import smtesting
from paraview.vtk import dataset_adapter

servermanager.ToggleProgressPrinting()

hasnumpy = True
try:
  from numpy import *
except ImportError:
  hasnumpy = False

if not hasnumpy :
   raise smtesting.TestError("NumPy library is not found")

# This function prints an array of scalars/vectors/tensors for display. Since
# it will be called with a loop, it offers the option to display the entire
# array or just the first few tuples. This is mainly used for debug.
def print_array (vtkarray, verbose=0) :
    ntuples = min(3, vtkarray.GetNumberOfTuples())
    if verbose : ntuples = max(3, vtkarray.GetNumberOfTuples())

    ncomps  = min(6, vtkarray.GetNumberOfComponents())
    if verbose : ncomps  = max(6, vtkarray.GetNumberOfComponents())

    for i in range(ntuples) :
        for j in range(ncomps) :
            print "%+12.6f" % vtkarray.GetComponent(i, j),
            sys.stdout.flush()
        print ''
        sys.stdout.flush()

# test0 tests all python programmable filters that work with data array
# associated with points/cells. The data array could be an array of
# scalars/vectors/tensors depending on the particular filter. Test will be
# applied to all combinations of points/cells and scalars/vectors/tensors for
# which a filter is well defined.
#
# @0 : name of the filter
# @1 : types of array that the filter is well defined
# @2 : number of operands of the filter
# @3 : places where the data array can be associated
# @4 : debug flag controlling human readable outputs
#      0 - no output
#      1 - output query, association and types
#      2 - output 1 and a brief display of the result array
#      3 - output 2 and a complete display of the result array
def test0 (query, types=[], noperands=1, associations=[], debug=1, testComposite=False) :
    if debug :
       print ''
       print query, associations, types, "Composite=%s"%testComposite
       sys.stdout.flush()

    # Validity checks
    if '' == query : return

    if 0 == len(types) : return
    for any in types :
        if any not in ['scalar', 'vector', 'tensor'] : return

    if 1 != noperands and 2 != noperands: return

    if 0 == len(associations) : return
    for any in associations :
        if any not in ['point', 'cell'] : return

    compositeOptions = [ False ]
    if testComposite:
        compositeOptions.append(True)

    # Python Calculator test
    for association in associations :
        for type in types :
            if   'scalar' == type : operand = 'Normals[:,0]'
            elif 'vector' == type : operand = 'Normals'
            elif 'tensor' == type : operand = 'VectorGradient'
            else : continue # Ideally, we should never come to here

            if 2 == noperands : operand = operand + ', ' + operand

            # Actual expression of the query
            expr = "%s(%s)" % (query, operand)
            if debug :
               sys.stdout.flush()

            datasource = Sphere()
            if 'tensor' == type :
                datasource = ComputeDerivatives()
                datasource.Vectors = ['Normals']

            if 'cell' == association :
                datasource = PointDatatoCellData()
            else:
                datasource = CellDatatoPointData()

            for composite in compositeOptions:
                if debug :
                    print '  PC', association.capitalize()[0], type.capitalize()[0], "Composite" if composite else "Sphere", expr
                    sys.stdout.flush()
                if composite:
                    otherSphere = Sphere()
                    if 'tensor' == type :
                        otherSphere = ComputeDerivatives()
                    if 'cell' == association :
                        otherSphere = PointDatatoCellData()
                    else :
                        otherSphere = CellDatatoPointData()
                    datasource = GroupDatasets( Input=[ datasource, otherSphere ] )

                SetActiveSource(datasource)

                filter = PythonCalculator(Expression=expr, ArrayAssociation=('cell'==association))
                filter.UpdatePipeline()

                if debug > 1 :
                    output = servermanager.Fetch(filter)
                    if   'point' == association :
                        print_array(output.GetPointData().GetArray("result"), debug>2)
                    elif 'cell'  == association :
                        print_array(output.GetCellData().GetArray("result"), debug>2)

    # Programmable Filter test
    for association in associations :
        for type in types :
            datasource = association.capitalize()
            base = "inputs[0].%sData['%%s']" % datasource
            if   'scalar' == type : operand = (base%"Normals") + "[:,0]"
            elif 'vector' == type : operand = base%"Normals"
            elif 'tensor' == type : operand = base%"VectorGradient"
            else : continue # Ideally, we should never come to here

            if 2 == noperands : operand = operand + ',' + operand

            # Actual expression of the query
            expr = "output.%sData.append(%s(%s), 'result')" % (datasource, query, operand)

            datasource = Sphere()
            if 'tensor' == type :
                datasource = ComputeDerivatives()
                datasource.Vectors = ['Normals']

            if 'cell' == association :
                datasource = PointDatatoCellData()
            else :
                datasource = CellDatatoPointData()

            for composite in compositeOptions:
                if debug :
                    print '  PF', association.capitalize()[0], type.capitalize()[0], "Composite" if composite else "Sphere", expr
                    sys.stdout.flush()
                if composite:
                    otherSphere = Sphere()
                    if 'tensor' == type :
                        otherSphere = ComputeDerivatives()
                    if 'cell' == association :
                        otherSphere = PointDatatoCellData()
                    else :
                        otherSphere = CellDatatoPointData()
                    datasource = GroupDatasets( Input=[ datasource, otherSphere ] )

                    SetActiveSource(datasource)

                # Unlike Python Calculator, the data association has been clarified
                # directly in the expression.
                filter = ProgrammableFilter(Script=expr)
                filter.UpdatePipeline()

                if debug > 1 :
                    output = servermanager.Fetch(filter)
                    if   'point' == association :
                        print_array(output.GetPointData().GetArray("result"), debug>2)
                    elif 'cell'  == association :
                        print_array(output.GetCellData().GetArray("result"), debug>2)

# test1 tests all python programmable filters that work with datasets directly.
#
# @0 : name of the filter
# @1 : types of geometries for which the query is well defined
# @2 : places to which the query can be issued
# @3 : debug flag controlling human readable outputs
#      0 - no output
#      1 - output query, association and types
#      2 - output 1 and a brief display of the result array
#      3 - output 2 and a complete display of the result array
def test1 (query, types=[], associations=[], debug=1) :
    if debug :
       print ''
       print query, types, associations
       sys.stdout.flush()

    # Validity checks
    if '' == query : return

    if 0 == len(types) : return
    for any in types :
        if any not in ['trig', 'quad', 'tet', 'hex'] : return

    if 0 == len(associations) : return
    for any in associations :
        if any not in ['point', 'cell'] : return

    # Python Calculator tests
    for association in associations :
        for type in types :
            if   'trig' == type : Sphere()
            elif 'quad' == type : Box()
            elif 'tet'  == type :
                Wavelet()
                Tetrahedralize()
            elif 'hex'  == type :
                # I don't know how to generate mesh composed of hexahedra, instead
                # just read in a dataset directly.
                file = os.path.join(smtesting.DataDir, "Testing/Data/multicomb_0.vts")
                reader = servermanager.sources.XMLStructuredGridReader(FileName=file)
            else : continue # Ideally, we should never come to here

            # Actual expression of the query
            expr = query + "(inputs[0])"
            if debug :
               print '  PC', association.capitalize()[0], type.capitalize()[0], expr
               sys.stdout.flush()

            if 'point' == association : filter = PythonCalculator(Expression=expr)
            else : filter = PythonCalculator(Expression=expr, ArrayAssociation=1)
            if 'hex' == type : filter.Input = reader
            filter.UpdatePipeline()

            if debug > 1 :
                if 'point' == association :
                    array = servermanager.Fetch(filter).GetPointData().GetArray("result")
                else : array = servermanager.Fetch(filter).GetCellData().GetArray("result")
                if array : print_array(array, debug>2)

    # Programmable Filter tests
    for association in associations :
        for type in types :
            if   'trig' == type : Sphere()
            elif 'quad' == type : Box()
            elif 'tet'  == type :
                Wavelet()
                Tetrahedralize()
            elif 'hex'  == type :
                dir = smtesting.DataDir
                file = os.path.join(dir, "Testing/Data/multicomb_0.vts")
                reader = servermanager.sources.XMLStructuredGridReader(FileName=file)
            else : continue

            # Actual expression of the query
            expr = "output.%sData.append(%s(inputs[0]), 'result')" % (association.capitalize(), query)
            if debug :
               print '  PF', association.capitalize()[0], type.capitalize()[0], expr
               sys.stdout.flush()

            # Unlike Python Calculator, the data association has been clarified
            # directly in the expression.
            filter = ProgrammableFilter(Script = expr)
            if 'hex' == type : filter.Input = reader
            filter.UpdatePipeline()

            if debug > 1 :
                if 'point' == association :
                    array = servermanager.Fetch(filter).GetPointData().GetArray("result")
                else : array = servermanager.Fetch(filter).GetCellData().GetArray("result")
                if array : print_array(array, debug>2)

def main () :
    # The arguments should be quite self-explanatory. In cases where they are
    # not, additional comments will be given.
    test0('abs',          ['scalar', 'vector', 'tensor'], 1, ['point', 'cell'])
    test1('area',         ['trig', 'quad'], ['cell'])
    test1('aspect',       ['trig', 'quad', 'tet'], ['cell'])
    test1('aspect_gamma', ['tet'], ['cell'])
    test0('cross',        ['vector'], 2, ['point', 'cell'])
    test0('curl',         ['vector'], 1, ['point', 'cell'])
    test1('diagonal',     ['hex'], ['cell'])
    test0('det',          ['tensor'], 1, ['point', 'cell'])
    test0('determinant',  ['tensor'], 1, ['point', 'cell'])
    test0('dot',          ['scalar', 'vector'], 2, ['point', 'cell'])
    test0('eigenvalue',   ['tensor'], 1, ['point', 'cell'])
    test0('eigenvector',  ['tensor'], 1, ['point', 'cell'])

    # The global_* requires Batch tests
    test0('global_mean', ['scalar', 'vector', 'tensor'], 1, ['point', 'cell'])
    test0('global_max',  ['scalar', 'vector', 'tensor'], 1, ['point', 'cell'])
    test0('global_min',  ['scalar', 'vector', 'tensor'], 1, ['point', 'cell'])
    test0('global_mean', ['scalar'], 1, ['point', 'cell'], testComposite=True)
    test0('global_max',  ['scalar'], 1, ['point', 'cell'], testComposite=True)
    test0('global_min',  ['scalar'], 1, ['point', 'cell'], testComposite=True)

    test0('gradient',       ['scalar', 'vector'], 1, ['point', 'cell'])
    test0('inverse' ,       ['tensor'], 1, ['point', 'cell'])
    test1('jacobian',       ['quad', 'tet', 'hex'], ['cell'])
    test0('laplacian',      ['scalar'], 1, ['point', 'cell'])
    test0('max',            ['scalar', 'vector', 'tensor'], 1, ['point', 'cell'])
    test0('min',            ['scalar', 'vector', 'tensor'], 1, ['point', 'cell'])
    test0('mean',           ['scalar', 'vector', 'tensor'], 1, ['point', 'cell'])
    test0('max',            ['scalar'], 1, ['point', 'cell'], testComposite=True)
    test0('min',            ['scalar'], 1, ['point', 'cell'], testComposite=True)
    test0('mean',           ['scalar'], 1, ['point', 'cell'], testComposite=True)
    test1('max_angle',      ['trig', 'quad'], ['cell'])
    test1('min_angle',      ['trig', 'quad'], ['cell'])
    test0('mag',            ['scalar', 'vector'], 1, ['point', 'cell'])
    test1('shear',          ['quad', 'hex'], ['cell'])
    test1('skew',           ['quad', 'hex'], ['cell'])
    test0('strain',         ['vector'], 1, ['point', 'cell'])
    test1('surface_normal', ['trig'], ['cell'])
    test0('trace',          ['tensor'], 1, ['point', 'cell'])
    test1('volume',         ['tet', 'hex'], ['cell'])
    test0('vorticity',      ['vector'], 1, ['point', 'cell'])

    # note this is the only test1 that applied on points.
    test1('vertex_normal',  ['trig'], ['point'])

    # these don't work due to division by zero or taking the log of zero
    #test0('ln',             ['scalar', 'vector', 'tensor'], 1, ['point', 'cell'])
    #test0('log',            ['scalar', 'vector', 'tensor'], 1, ['point', 'cell'])
    #test0('log10',          ['scalar', 'vector', 'tensor'], 1, ['point', 'cell'])
    #test0('norm',           ['scalar', 'vector'], 1, ['point', 'cell'])

if __name__ == "__main__" :
   smtesting.ProcessCommandLineArguments()
   main()
