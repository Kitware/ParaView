/**
 * ParaViewWeb JavaScript Library.
 * 
 * This module focus on performance statistics.
 * 
 * @class paraview.statistics
 */
(function (GLOBAL, $) {

    var module = {};

    /**
     * @member paraview.statistics
     *
     * Create and returns a ViewportStatistics object for a particular
     * viewport. The returned object can be used to obtain various statistics
     * about the viewport's performance such as frame-rate, average size of
     * images delivered, etc.
     * 
     * @param {pv.Viewport} viewport
     * Viewport to monitor.
     *
     * @return {pv.ViewportStatistics}
     */
    function createViewportStatistics(viewport) {
        if (viewport === null) {
            throw "'viewport' must be provided.";
        }

        // Internal fields
        var timeStamp = null,
        counter = 0,
        renderTime = 0,
        renderTimeSum = 0,
        roundTripTime = 0,
        maxRoundTripTime = 0,
        serverProcessingTime = 0,
        maxServerProcessingTime = 0,
        trueLoadTimeStart = 0,
        trueLoadTime = 0,
        htmlElement = viewport.getHTMLElement();

        function renderStart(event) {
            timeStamp = (new Date()).getTime();
        }

        function renderEnd(event) {
            var ms = (new Date()).getTime() - timeStamp;
            renderTimeSum += ms;
            renderTime = ms;
            counter += 1;
        }
        
        function roundTripEvent(event) {
            roundTripTime = event.time;
            maxRoundTripTime = (roundTripTime > maxRoundTripTime) ? roundTripTime : maxRoundTripTime;
        }
        
        function serverProcessingEvent(event) {
            serverProcessingTime = event.time;
            maxServerProcessingTime = (serverProcessingTime > maxServerProcessingTime) ? serverProcessingTime : maxServerProcessingTime;
        }
        
        function startLoading() {
            trueLoadTimeStart = new Date().getTime();
        }
        
        function stopLoading() {
            trueLoadTime = new Date().getTime() - trueLoadTimeStart;
        }

        function start() {
            htmlElement.on("render-start", renderStart);
            htmlElement.on("render-end", renderEnd);
            htmlElement.on("round-trip", roundTripEvent);
            htmlElement.on("server-processing", serverProcessingEvent);
            htmlElement.on("start-loading", startLoading);
            htmlElement.on("stop-loading", stopLoading);
        }

        function stop() {
            htmlElement.off("render-start", renderStart);
            htmlElement.off("render-end", renderEnd);
            htmlElement.off("round-trip", roundTripEvent);
            htmlElement.off("server-processing", serverProcessingEvent);
            htmlElement.off("start-loading", startLoading);
            htmlElement.off("stop-loading", stopLoading);
        }
        function reset() {
            counter = 0;
            renderTimeSum = 0;
        }
        function benchmark(ondone) {
            stop();
            reset();

            var max_iterations = 100;
            function renderEndBenchmark(event) {
                renderEnd(event);
                if (counter < max_iterations) {
                    viewport.render();
                } else {
                    if (ondone) {
                        ondone();
                    }
                    htmlElement.off("render-start", renderStart);
                    htmlElement.off("render-end", renderEndBenchmark);
                    htmlElement.off("round-trip", roundTripEvent);
                    htmlElement.off("server-processing", serverProcessingEvent);
                    htmlElement.on("start-loading", startLoading);
                    htmlElement.on("stop-loading", stopLoading);
                }
            }

            htmlElement.on("render-start", renderStart);
            htmlElement.on("render-end", renderEndBenchmark);
            htmlElement.on("round-trip", roundTripEvent);
            htmlElement.on("server-processing", serverProcessingEvent);
            htmlElement.off("start-loading", startLoading);
            htmlElement.off("stop-loading", stopLoading);
            viewport.render();
        }

        start();
        return {
            /**
             * @class pv.ViewportStatistics
             * Object maintain statistics gathered from a viewport.
             * A ViewportStatistics instance keeps monitors and collects
             * information about a specific viewport, including how long an
             * image request takes on average, the size of data received per
             * render, etc.
             *
             * A ViewportStatistics instance can be obtained as follows:
             *      var stats = paraview.createViewportStatistics(viewport);
             * One can display the stats in a viewport as follows:
             *      viewport.showStatistics(stats);
             */
            /**
             * Returns the framerate for the last rendered frame.
             * @return {Number} Computed frame rate.
             */
            frameRate: function () {
                return 1000.0 / renderTime;
            },
            /**
             * Returns the average framerate.
             * @return {Number} Computed average frame rate.
             */
            averageFrameRate: function () {
                return 1000.0 * counter / renderTimeSum;
            },
            /**
             * Returns the round trip time in ms.
             * @return {Number} Computed round trip without the server processing time.
             */
            roundTrip: function () {
                return roundTripTime;
            },
            /**
             * Returns the server processing time.
             * @return {Number} time taken on the server side to build a reply in ms.
             */
            serverWorkTime: function () {
                return serverProcessingTime;
            },
            /**
             * @return {Number} the maximum round trip time.
             */
            maxRoundTrip: function () {
                return maxRoundTripTime;
            },
            /**
             * @return {Number} the maximum server working time.
             */
            maxServerWorkTime: function () {
                return maxServerProcessingTime;
            },
            /**
             * @return {Number} the minimum framerate that can be achieved based
             * on the Max server work time and Max round trip.
             */
            minFrameRate: function () {
                return 1000.0 / (maxServerProcessingTime + maxRoundTripTime + 1);
            },
            /**
             * @return {Number} the true loading time in ms between the base64
             * reception to the onLoad callback.
             */
            trueLoadTime: function () {
                return trueLoadTime;
            },
            /**
             * Resets the statics collected.
             */
            reset: function () {
                counter = 0;
                renderTimeSum = 0;
                maxServerProcessingTime = 0;
                maxRoundTripTime = 0;
                return this;
            },
            /**
             * Begin monitoring the viewport to collect statistics information.
             * It's not necessary to call this function, unless the monitoring
             * was explicitly stopped by calling stop().
             */
            start: function () {
                start();
                return this;
            },
            /**
             * ViewportStatistics keeps on collecting statistics from the
             * viewport. To stop the gathering of this information, use this
             * method.
             */
            stop: function () {
                stop();
                return this;
            },
            /**
             * Runs a suite of tests to detemine statistics.
             */
            benchmark : function (ondone) {
                benchmark(ondone);
                return this;
            }
        };
    }

    // ----------------------------------------------------------------------
    // Init paraview module if needed
    // ----------------------------------------------------------------------
    if (GLOBAL.hasOwnProperty("paraview")) {
        module = GLOBAL.paraview || {};
    } else {
        GLOBAL.paraview = module;
    }

    // ----------------------------------------------------------------------
    // Export internal methods to the paraview module
    // ----------------------------------------------------------------------
    module.createViewportStatistics = function (viewport) {
        return createViewportStatistics(viewport);
    };
}(window, jQuery));
