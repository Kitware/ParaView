(function($, GLOBAL, undefined) {
    function Legend(args) {
        // extends Rickshaw's Legend to add support to update the legend with
        // the graph updates. This is essential since in ParaView the available
        // series (names/count) may change on each update.
        var legend = new Rickshaw.Graph.Legend(args);
        var element = this.element = args.element;
        var graph = this.graph = args.graph;
        var self = this;

        this.lines = legend.lines;

        function update() {
            // clear current.
            $(element).children("ul").empty();
            legend.lines = []
            legend.shelving = self.shelving;
            legend.highlighter = self.highlighter;

            self.lines = legend.lines;

            var series = graph.series.map( function(s) { return s } )
            if (!args.naturalOrder) {
                series = series.reverse();
            }
            series.forEach(function(s) {
                    legend.addLine(s);
            });
        }

        graph.onUpdate( function() {
            update();
        });
    }

    // converts marshalled plot data into a JavaScript object that the
    // Rickshaw plots expect.
    function processPlotData(data, x_axis, mask_array) {
        var palette = new Rickshaw.Color.Palette();
        var series = [];
        var disabled_regex = /(.*[(]\d+[)].*)/;
        var purged_regex = /(^arc_length$)|(.*Id$)|(.*ID$)/;

        for (var idx=0; idx < data.length; data++) {
            var dataset = data[idx];
            var mask_index = -1;
            if (mask_array) {
                dataset.headers.some(function(element, index) {
                    if (element === mask_array) {
                        mask_index = index;
                        return true; // breaks the loop
                    }
                    return false; // continue the loop.
                });
            }

            for (var cc=0; cc < dataset.headers.length; cc++) {
                var item = {};
                item.name = dataset.headers[cc];
                // set default enabled state for the series.
                if (cc === mask_index) {
                    // don't show mask array.
                    item.disabled = true;
                } else if (disabled_regex.test(item.name)) {
                    item.disabled = true;
                } else if (purged_regex.test(item.name)) {
                    continue;
                }
                item.color = palette.color();
                item.data = [];

                for (var kk=0; kk < dataset.data.length; kk++) {
                    var x = (x_axis >= 0? dataset.data[kk][x_axis] : kk);
                    var y = dataset.data[kk][cc];
                    var mask = (mask_index >=0? dataset.data[kk][mask_index] : true)
                    var valid = (mask !== "_nan_" && mask);
                    if (x == "_nan_") { x = Number(0); valid = false; }
                    if (y == "_nan_") { valid = false; }
                    if (!valid) {
                      // Rickshaw uses y !== null to determine if the data-point
                      // is "defined".
                      y = null;
                    }
                    var value = { "x" : x, "y" : y};
                    item.data.push(value);
                }

                if (item.data.length > 0) {
                    series.push(item);
                }
            }
        }
        return { data: series };
    }

    // FIXME: At some point, I'll convert this to remove the dependency on
    // jQueryUI, but for now we'll let it be.

    $.widget("pv.graph", {
        version : "0.0.1",

        // default options
        options : {
            id : null,
            topicUri : "vtk.event.probe.data.changed",
            connection : null,
            mask: "Element Status"
        },

        _create : function() {
            var self = this;
            if (self.options.connection && self.options.topicUri) {
                self.options.connection.session.subscribe(
                    self.options.topicUri,
                    function() { self.updatePlotData(); });
            }

            // add a class for theming.
            self.element.addClass("pv-graph");

            var axisSize = 50;
            self.elementYAxis = $("<div></div>")
                .appendTo(self.element)
                .css("position", "absolute")
                .css("left","0px")
                .css("top", "0px")
                .width(axisSize)
                .height(self.element.height());
            self.elementChart = $("<div></div>")
                .addClass("pv-elementChart")
                .appendTo(self.element)
                .css("position", "relative")
                .css("left", axisSize + "px")
                .width(self.element.width() - axisSize)
                .height(self.element.height() - axisSize);
            self.elementXAxis = $("<div></div>")
                .appendTo(self.element)
                .css("position", "relative")
                .css("left", axisSize + "px")
                .width(self.element.width() - axisSize)
                .height(axisSize);
            self.elementLegend = $("<div></div>")
                .appendTo(self.element)
                .css({
                    position: "absolute",
                    top: "30px",
                    right: "30px"
                })
                .addClass("paraview_graph_legend")

            self.series = [{
                    data: [ { x: 0, y: 40}, {x: 1, y: 49} ],
                    color: 'steelblue'
                }];
            self.graph = new Rickshaw.Graph({
                element : self.elementChart[0],
                renderer: "line",
                series : self.series,
                interpolation: "linear"
            });
            self.resize();
            self.updatePlotData();

            self.hoverDetail = new Rickshaw.Graph.HoverDetail({
                graph : self.graph,
                xFormatter: function (x) {
                    return x === null ? x : x.toFixed(2);
                }
            });
            self.xAxis = new Rickshaw.Graph.Axis.X({
                graph: self.graph,
                element: self.elementXAxis[0],
                orientation: "bottom"
            });
            self.yAxis = new Rickshaw.Graph.Axis.Y({
                graph: self.graph,
                element: self.elementYAxis[0],
                orientation: "left"
            });
            self.legend = new Legend({
                graph: self.graph,
                element: self.elementLegend[0]
            });
            self.shelving = new Rickshaw.Graph.Behavior.Series.Toggle({
                graph: self.graph,
                legend: self.legend
            });
            self.legend.shelving = self.shelving;
        },

        _destroy : function() {
            var self = this;
            self.graph = undefined;
            self.element.removeClass("pv-graph");
            self.element.empty();
        },

        updatePlotData : function() {
            var self = this;
            if (self.options.connection == null ||
                self.options.connection.session == null) { return; }

            var session = self.options.connection.session;
            session.call("pv.data.prober.probe.data").then(function(result) {
                var data = processPlotData(result, -1, self.options.mask);
                var key_map = {};
                self.series.forEach(function(d) {
                    key_map[d.name] = d.disabled;
                });
                for (var cc=self.series.length-1; cc >=0; cc--) {
                    self.series.pop();
                }
                $.extend(self.series, data.data);
                self.series.forEach(function(d) {
                    var disabled = key_map[d.name];
                    if (disabled !== undefined) {
                        d.disabled = disabled;
                    }
                });
                self.graph.update();
            });
        },

        configureGraph : function(args) {
            self = this;
            self.graph.configure(args);
            self.graph.render();
        },

        resize: function () {
            self = this;
            if (self.timer) {
                GLOBAL.clearTimeout(self.timer);
            }
            self.timer = GLOBAL.setTimeout(function(){
                var size = {width: self.element.width(), height: self.element.height() };
                var axisSize = 50;
                console.log(size + "");
                self.elementYAxis
                    .height(size.height);
                self.elementChart
                    .width(size.width - axisSize)
                    .height(size.height - axisSize);
                self.elementXAxis
                    .width(size.width - axisSize);
                size.width -= 50;
                size.height -= 50;
                if (self.graph) {
                    self.graph.configure(size);
                    self.graph.update();
                }
            }, 300);
        }

    });
})(jQuery,window);
