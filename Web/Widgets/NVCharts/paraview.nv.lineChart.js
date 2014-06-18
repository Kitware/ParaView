// ParaView Web Line Chart (NVD3-based)
// Provides implementation for a line-chart based on NVD3 (http://nvd3.org)
(function($, undefined) {
    $.widget( "pv.lineChart", {
        version : "0.0.1",

        // default options
        options : {
            id : null,
            topicUri : "event:probeDataChanged",
            connection: null
        },

        _create : function() {
            var self = this;

            if (self.options.connection) {
                self.options.connection.session.subscribe(
                    self.options.topicUri,
                    function() { self.updatePlotData(); });
            }

            // add a class for theming.
            self.element.addClass("pv-linechart");
            self.svg = self.element[0];

            nv.addGraph(function() {
                return self._createChart();
            });
        },

        _destroy : function() {
            var self = this;

            self.chart = undefined;

            self.element.removeClass("pv-linechart");

            // remove all child elements.
            self.element.empty();
        },

        _createChart : function() {
            var self = this;

            var chart = nv.models.lineChart();
            if (self.options.connection == null) {
                chart.noData("No Connection Provided");
            }
            var svgd3 = d3.select(self.svg);
            svgd3.call(chart);
            chart.showLegend(false);

            // handle window resize.
            nv.utils.windowResize(function() {
                chart.update();

                // resize legend manually.
                chart.legend.width(chart.lines.width());
                d3.select(self.element.find("g .nv-legendWrap")[0])
                    .call(chart.legend);
            });

            // handle clicks on legend to toggle series visibility.
            chart.legend.dispatch.on('legendClick.pv', function(d,i) {
                d.disabled = !d.disabled;
                for (var j=0; j < d.data.length; j++) {
                    d.data[j].disabled = d.disabled;
                }
                // re-render chart.
                self.chart.update();

                // re-render legend.
                d3.select(self.element.find("g .nv-legendWrap")[0])
                    .call(self.chart.legend);
            });

            self.chart = chart;
            return chart;
        },

        updatePlotData : function() {
            var self = this;
            if (self.options.connection == null ||
                self.options.connection.session == null) {
                return;
            }

            var session = self.options.connection.session;
            session.call("pv.data.prober.probe.data").then(function(result) {
                var selection = d3.select(self.svg);

                // save data selection, if any.
                var legend_map = {};
                if (selection.datum()) {
                  var datum = selection.datum();
                  for (var cc=0; cc < datum.length; cc++) {
                    legend_map[datum[cc].key] = datum[cc].disabled;
                  }
                }

                var data = vtkWeb.processPlotData(result, -1);

                var vectorRegEx = /.*[(]\d+[)].*/;
                for (var cc=0; cc < data.data.length; cc++) {
                  var key = data.data[cc].key;
                  if (legend_map[key]) {
                    data.data[cc].disabled = true;
                  } else if (legend_map[key] === undefined && vectorRegEx.test(key)) {
                    data.data[cc].disabled = true;
                  }
                }

                for (var cc=0; cc < data.legend.length; cc++) {
                  var key = data.legend[cc].key;
                  if (legend_map[key]) {
                    data.legend[cc].disabled = true;
                  } else if (legend_map[key] === undefined && vectorRegEx.test(key)) {
                    data.legend[cc].disabled = true;
                  }
                }

                // set the axis titles.
                self.chart.xAxis.axisLabel("Probe Index");

                selection
                    .datum(data.data)
                   .transition().duration(500)
                    .call(self.chart);
                self.chart.legend.width(self.chart.lines.width());
                d3.select(self.element.find("g .nv-legendWrap")[0])
                    .datum(data.legend)
                   .transition().duration(500)
                    .call(self.chart.legend);
            });
        }
    });
})(jQuery);
