/**
 * ParaView Web Charting Utilities
 *
 * This module provides utility functions use for charting/plotting data.
 *
 * @class vtkWeb.helper
 */
(function(GLOBAL, $) {
    // ----------------------------------------------------------------------
    // Init vtkWeb module if needed
    // ----------------------------------------------------------------------
    if (GLOBAL.hasOwnProperty("vtkWeb")) {
        module = GLOBAL.vtkWeb || {};
    } else {
        GLOBAL.vtkWeb = module;
    }

    // converts marshalled plot data into a JavaScript object that the
    // nvd3 plots expect. To handle "nan" values, we return separate
    // data for the plot and the legend.
    function processPlotData(data, x_axis) {
        var color = nv.utils.defaultColor();
        var result = [];
        var legend = [];

        for (var idx=0; idx < data.length; data++) {
            var dataset = data[idx];
            for (var cc=0; cc < dataset.headers.length; cc++) {
                var item = {};
                item.key = dataset.headers[cc];
                item.color = color(dataset, cc);
                item.values = [];

                var legendItem = {};
                legendItem.key = item.key;
                legendItem.color = item.color;
                legendItem.data = [];

                for (var kk=0; kk < dataset.data.length; kk++) {
                    var x = (x_axis >= 0? dataset.dataset[kk][x_axis] : kk);
                    var y = dataset.data[kk][cc];
                    if (x == "_nan_" || y == "_nan_") {
                        // break the curve.
                        if (item.values.length > 0) {
                            legendItem.data.push(item);
                            var new_item = {};
                            new_item.key = item.key;
                            new_item.values = [];
                            new_item.color = item.color;
                            item = new_item;
                        }
                        continue;
                    }

                    var value = { "x" : x, "y" : y};
                    item.values.push(value);
                }
                if (legendItem.data.length > 0) {
                    for (var j=0; j < legendItem.data.length; j++) {
                        result.push(legendItem.data[j]);
                    }
                    legend.push(legendItem);
                }
            }
        }
        return { data: result, legend : legend };
    }

    /**
     * @member vtkWeb.helper
     * @method processPlotData
     * @param {Object} data
     * Object representing a vtkTable.
     * @param {String|null|undefined} independent_variable
     * Name of the variable to be treated as the 'x' variable
     */
     module.processPlotData = function (data, independent_variable) {
        return processPlotData(data, independent_variable);
     }

})(window, jQuery);
