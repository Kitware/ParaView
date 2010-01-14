

class Error(Exception): pass

NODE_VARIABLE = "POINT_DATA"
ELEMENT_VARIABLE = "CELL_DATA"
GLOBAL_VARIABLE = "FIELD_DATA"

EXODUS = 1
SPYPLOT = 2

STANDARD_COLORS = [
    [ 0.933, 0.000, 0.000],     # Red
    [ 0.000, 0.804, 0.804 ],    # Cyan
    [ 0.000, 1.000, 0.000 ],    # Green
    [ 0.804, 0.804, 0.000 ],    # Yellow
    [ 0.647, 0.165, 0.165 ],    # Brown
    [ 0.804, 0.569, 0.620 ],    # Pink
    [ 0.576, 0.439, 0.859 ],    # Purple
    [ 0.804, 0.522, 0.000 ],    # Orange
    ]

class Variable(object):
    def __init__(self, source, name, type, component):
        self._source = source
        self._name = name
        self._type = type
        self._component = component

    def _get_name(self): return self._name
    def _get_source(self): return self._source
    def _get_type(self): return self._type
    def _get_component(self): return self._component
    def _get_decorated_name(self):
        suffix = ["","X","Y","Z"]
        return "%s%s" % (self.name, suffix[self.component+1])
    def _get_type_name(self):
        if self.type == NODE_VARIABLE: return "NODE"
        if self.type == ELEMENT_VARIABLE: return "ELEMENT"
        if self.type == GLOBAL_VARIABLE: return "GLOBAL"
        return "UNKNOWN_TYPE"

    def _get_info(self):
        r = self._source
        itr = zip([NODE_VARIABLE, ELEMENT_VARIABLE, GLOBAL_VARIABLE],
                  [r.PointData, r.CellData, r.FieldData])
        for var_type, var_data in itr:
            if self.type == var_type:
                return var_data[self.name]
        return None

    source = property(_get_source, None, None, "The source proxy that has this variable")
    name = property(_get_name, None, None, "The variable name used by the source.")
    type = property(_get_type, None, None, "The variable type.")
    component = property(_get_component, None, None, "The variable component.  -1 for magnitude.")
    type_name = property(_get_type_name, None, None, "Get the variable type as a string")
    decorated_name = property(_get_decorated_name, None, None, "The variable name with "
                                                               "X, Y, or Z possibly appended.")
    info = property(_get_info, None, None, "Get the ArrayInformation object for the variable.")



def find_variable(source, name):
    """Given a source and a case insensitive variable name, look for the
    variable as a node, element, or global variable.  If found, return a
    new Variable instance with the variable name (the real name, may be
    different from the name given to this method), type, and component.
    If the variable is not found return None.  As a side effect, if the
    variable is found but it is not currently loaded by the source (reader), the
    variable will be loaded.  When passing a variable name to this
    method you can pass VARX or VARY to mean VAR(0) or VAR(1) for example."""

    if source.DatabaseType == EXODUS:
        itr = zip([NODE_VARIABLE, ELEMENT_VARIABLE, GLOBAL_VARIABLE],
                [source.PointVariables, source.ElementVariables, source.GlobalVariables])

    elif source.DatabaseType == SPYPLOT:
        itr = zip([ELEMENT_VARIABLE], [source.Input.CellArrays])

    for var_type, var_list in itr:
        realname = find_insensitive(name, var_list.Available)
        component = -1
        if not realname:
            for component_index, suffix in zip([0, 1, 2], ["x", "y", "z"]):
                if name.lower().endswith(suffix):
                    realname = find_insensitive(name[:-1], var_list.Available)
                    component = component_index  
        if realname:
            if realname not in var_list:
                var_list.SetData(var_list[:] + [realname])
            return Variable(source, realname, var_type, component)
    return None

def find_insensitive(str, lst):
    """
    Finds string str in list of strings lst ignoring case and spaces.  If the string is
    found, returns the actual value of the string.  Otherwise, returns None.
    """
    str = str.upper().replace(" ", "")
    for entry in lst:
        if entry.upper().replace(" ", "") == str:
            return entry
    return None

def print_blot_error(err):
    """Print an instance of Error using blot formatting."""
    print " *** ERROR - ", err

def print_blot_warning(msg):
    """Print a warning message."""
    print " *** WARNING - ", msg

