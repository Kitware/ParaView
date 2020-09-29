r"""Module used by vtkCDBWriter"""
from ..tpl import cinemasci

def write(fname, vtktable):
    cdb = cinemasci.new("cdb", { "path" : fname})
    if not cdb.initialize(dirExistCheck=False):
        raise RuntimeError("Failed to initialize dbase")
    for row in range(vtktable.GetNumberOfRows()):
        entry = {}
        for col in range(vtktable.GetNumberOfColumns()):
            value = vtktable.GetValue(row, col).ToString()
            if value:
                entry[vtktable.GetColumnName(col)] = value
        cdb.add_entry(entry)
    cdb.finalize()
    return True
