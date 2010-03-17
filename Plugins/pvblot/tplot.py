import blot_common
import paraview.simple
smp = paraview.simple

class Error(Exception): pass

# Some constants
NODE_VARIABLE = blot_common.NODE_VARIABLE
ELEMENT_VARIABLE = blot_common.ELEMENT_VARIABLE
GLOBAL_VARIABLE = blot_common.GLOBAL_VARIABLE

TYCURVE = 1
XYCURVE = 2

STANDARD_CHART_COLORS = [
    [ 0.933, 0.000, 0.000],     # Red
    [ 0.000, 0.804, 0.804 ],    # Cyan
    [ 0.000, 1.000, 0.000 ],    # Green
    [ 0.647, 0.165, 0.165 ],    # Brown
    [ 0.804, 0.569, 0.620 ],    # Pink
    [ 0.576, 0.439, 0.859 ],    # Purple
    [ 0.804, 0.522, 0.000 ],    # Orange
    ]

_global_view = None

class TPlot(object):

    def __init__(self):

        """
        self.node_selection = \
            paraview.servermanager.sources.GlobalIDSelectionSource(FieldType="POINT")
        self.element_selection = \
            paraview.servermanager.sources.GlobalIDSelectionSource(FieldType="CELL")
        self.node_plot = smp.PlotSelectionOverTime(Selection=self.node_selection)
        self.element_plot = smp.PlotSelectionOverTime(Selection=self.element_selection)
        """
        global _global_view
        if not _global_view:
            _global_view = smp.CreateXYPlotView()
        self.view = _global_view
        self._curve_reps = dict()
        self._filters = []
        self.reset()
        pass

    def __del__(self):
        self.reset()
        

    def reset(self):
        self._curves = []
        self._overlay = False
        self._node_curves = []
        self._element_curves = []
        self._global_curves = []
        self._overlay = False
        self._xlabel = ""
        self._ylabel = ""

        self.view.AxisBehavior = [0, 0, 0, 0]
        self.view.AxisTitle = ['', '', '', '']
        self.view.Representations = []

        for k in self._curve_reps.keys():
            rep = self._curve_reps.pop(k)
            smp.Delete(rep)
        self._curve_reps = dict()

        for proxy in self._filters:
            smp.Delete(proxy)
        self._filters = list()


        self.view.Representations = []
        for r in self._curve_reps: smp.Delete(r)
        smp.Render(self.view)

    def _set_overlay(self, value=True):
        self._overlay = bool(value)
    def _get_overlay(self): return self._overlay
    overlay = property(_get_overlay, _set_overlay,
                       doc="Overlay decides if all curves are plotted together or not.")

    def add_curve(self, c):
        if not isinstance(c, Curve):
            raise TypeError("Argument is not a tplot curve.")
        c.curve_index = len(self._curves)
        self._curves.append(c)
        if c.var.type == GLOBAL_VARIABLE: self._global_curves.append(c)
        if c.var.type == NODE_VARIABLE: self._node_curves.append(c)
        if c.var.type == ELEMENT_VARIABLE: self._element_curves.append(c)


    def write_image(self, filename):
        self.view.WriteImage(filename)

    def get_curve_representation(self, c):
        rep = self._curve_reps.get(c.curve_index)
        if rep: return rep

        if c.var.type == GLOBAL_VARIABLE:
            return self.create_global_over_time_representation(c)
        else:
            return self.create_selection_over_time_representation(c)

    def create_global_over_time_representation(self, c):

        plot = smp.PlotGlobalVariablesOverTime(c.var.source)
        self._filters.append(plot)

        chart_variable_name = c.chart_variable_name
        chart_series_label = c.get_series_label()
        color = STANDARD_CHART_COLORS[0]

        rep = paraview.servermanager.CreateRepresentation(plot, self.view)
        self._curve_reps[c.curve_index] = rep
        paraview.servermanager.ProxyManager().RegisterProxy("representations", \
          "plot_rep_%d" % c.curve_index, rep)

        rep.CompositeDataSetIndex = 2
        rep.AttributeType = 6 # "Row Data"
        rep.Update()
        visibility = []
        for name in rep.GetProperty("SeriesNamesInfo"):
          visibility.append(name)
          if name == chart_variable_name: visibility.append("1")
          else: visibility.append("0")
        rep.SeriesVisibility = visibility
        rep.SeriesLineStyle = [chart_variable_name, "1"]
        rep.SeriesColor = [chart_variable_name, str(color[0]), str(color[1]), str(color[2])]
        rep.SeriesLabel = [chart_variable_name, chart_series_label]
        return rep


    def create_selection_over_time_representation(self, c):

        field_type = _get_field_type_for_variable(c.var)
        selection = smp.GlobalIDSelectionSource(FieldType=field_type)
        selection.GlobalIDs = [long(c.id)]
        plot = smp.PlotSelectionOverTime(c.var.source, Selection=selection)

        self._filters.append(selection)
        self._filters.append(plot)

        chart_variable_name = c.chart_variable_name
        chart_series_label = c.get_series_label()
        color = STANDARD_CHART_COLORS[0]

        rep = paraview.servermanager.CreateRepresentation(plot, self.view)
        self._curve_reps[c.curve_index] = rep
        paraview.servermanager.ProxyManager().RegisterProxy("representations", \
          "plot_rep_%d" % c.curve_index, rep)

        rep.CompositeDataSetIndex = 1
        rep.AttributeType = 6 # "Row Data"
        rep.Update()
        visibility = []
        for name in rep.GetProperty("SeriesNamesInfo"):
          visibility.append(name)
          if name == chart_variable_name: visibility.append("1")
          else: visibility.append("0")
        rep.SeriesVisibility = visibility
        rep.SeriesLineStyle = [chart_variable_name, "1"]
        rep.SeriesColor = [chart_variable_name, str(color[0]), str(color[1]), str(color[2])]
        rep.SeriesLabel = [chart_variable_name, chart_series_label]

        if c.type == XYCURVE:
            rep.XArrayName = c.chart_xvariable_name
            rep.UseIndexForXAxis = 0

        return rep


    def get_number_of_curves(self):
        return len(self._curves)

    def plot(self, curve_index=0):

        if self.overlay:
            curves_to_plot = self._curves
        elif curve_index < len(self._curves):
            curves_to_plot = [self._curves[curve_index]]
        else: return          
        
        view = self.view
        view.Representations = []

        if len(curves_to_plot) == 1:
            c = curves_to_plot[0]
            chart_title = c.get_description()
            y_axis_title = c.var.decorated_name
            x_axis_title = "TIME"
            if c.type == XYCURVE: x_axis_title = c.x_var.decorated_name
        else:
            # todo
            chart_title = ""
            y_axis_title = ""
            x_axis_title = ""

        view.ChartTitle = chart_title

        xlabel, ylabel = self._xlabel, self._ylabel
        if not xlabel: xlabel = x_axis_title
        if not ylabel: ylabel = y_axis_title


        view.AxisTitle = [ylabel, xlabel, '', '']

        count = 0
        for c in curves_to_plot:
            rep = self.get_curve_representation(c)
            view.Representations.append(rep)

            # Set a new color for the curve depending on the count
            color = STANDARD_CHART_COLORS[count % len(STANDARD_CHART_COLORS)]
            rep.SeriesColor = [c.chart_variable_name, str(color[0]), str(color[1]), str(color[2])]
            count += 1

        smp.Render(view)


    def _set_axis_range(self, axis_index, min_val, max_val):
        """Set the minimum and maximum values for the axis range.
        The Y axis is axis_index=0.  The X axis is axis_index=1."""
        view = self.view
        axis_name = ["Y", "X"][axis_index]
        if min_val is None:
            # Set to autoscale
            view.AxisBehavior[axis_index] = 0
            print " %s axis will be automatically scaled" % axis_name
        else:
            min_index, max_index = axis_index*2, axis_index*2 + 1
            view.AxisBehavior[axis_index] = 1
            view.AxisRange[min_index] = min_val

            # If max_val is not none then set it, otherwise just grab the value
            if max_val is not None:
                view.AxisRange[max_index] = max_val
            else: max_val = view.AxisRange[max_index]

            print " %s axis scaling: %f to %f" % (axis_name, min_val, max_val)


    def set_xscale(self, min_val=None, max_val=None):
        self._set_axis_range(1, min_val, max_val)

    def set_yscale(self, min_val=None, max_val=None):
        self._set_axis_range(0, min_val, max_val)

    def set_xlabel(self, label):
        self._xlabel = label

    def set_ylabel(self, label):
        self._ylabel = label


    def _unique_curve_ids(self, curve_list):
        """"Given a list of curves return a sorted list of the curve ids
        with duplicates removed."""
        uniq = dict()
        for c in curve_list:
            uniq.setdefault[c.id] = 0
        return uniq.keys().sort()

    def get_curves_with_variable_type(self, var_type):
        """Given a variable type (NODE_VARIABLE, ELEMENT_VARIABLE, GLOBAL_VARIABLE)
        return all the curves for variables of the given type."""
        return [c for c in self._curves if c.var.type == var_type]

    def get_node_ids(self):
        return self._unique_curve_ids(self.get_curves_with_variable_type(NODE_VARIABLE))

    def get_element_ids(self):
        return self._unique_curve_ids(self.get_curves_with_variable_type(ELEMENT_VARIABLE))

    def get_current_curve_mode(self):
        if not self._curves: return None
        return self._curves[0].type


    def print_show(self):
        count = 1
        for c in self._curves:
            print " Curve %3d: %s" % (count, c.get_description())
            count += 1


class Curve(object):

    def __init__(self, variable, id=None):
        self.var = variable
        self.id = id
        self.type = TYCURVE
        self.x_var = None
        self.chart_variable_name = self._get_chart_variable_name(self.var)


    def _get_chart_variable_name(self, var):
        component_suffix = str()
        if var.info.GetNumberOfComponents() > 1:
            component_names = ["Magnitude", "0", "1", "2"]
            component_suffix = " (%s)" % component_names[var.component+1]
        return "%s%s" % (var.name, component_suffix)

    def set_x_variable(self, var):
        self.type = XYCURVE
        self.x_var = var
        self.chart_xvariable_name = self._get_chart_variable_name(self.x_var)

    def get_description(self):
        if self.var.type == GLOBAL_VARIABLE:
            return "%s -vs- TIME" % self.var.decorated_name
        elif not self.x_var:
            return "%s at %s %3d -vs- TIME" % (self.var.decorated_name,
                                              self.var.type_name,
                                              self.id)
        else:
            return "%s at %s %3d -vs- %s at %s %3d" % (self.x_var.decorated_name,
                                                       self.x_var.type_name,
                                                       self.id,
                                                       self.var.decorated_name,
                                                       self.var.type_name,
                                                       self.id)

    def get_series_label(self):
        if self.var.type == GLOBAL_VARIABLE:
            return self.var.decorated_name
        else:
            return "%s %s %s" % (self.var.decorated_name,
                                 self.var.type_name,
                                 self.id)  

   

def _get_field_type_for_variable(var):
    if var.type == NODE_VARIABLE: return "POINT"
    if var.type == ELEMENT_VARIABLE: return "CELL"
