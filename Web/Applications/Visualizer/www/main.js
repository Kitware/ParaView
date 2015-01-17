(function (GLOBAL, $) {
    var session,
        viewport,
        pipeline,
        proxyEditor,
        settingsEditor,
        rvSettingsProxyId = null,
        infoManager,
        busyElement = $('.busy').hide(),
        notBusyElement = $('.not-busy').show(),
        busyCount = 0,
        paletteNameList = [],
        pipelineDataModel = { metadata: null, source: null, representation: null, view: null, sources: []},
        module = {},
        vcrPlayStatus = false,
        pipelineLoadedCallBack = null,
        error = function(e){workDone();console.log(e);};

    // ========================================================================
    // Main callback
    // ========================================================================

    function invalidatePipeline(arg) {
        if(arg && arg.hasOwnProperty('id')) {
            // Update active proxy in pipeline
            activeProxy(arg.id);
        }
        pipeline.trigger('pipeline-reload');
        workDone();
    }

    // ========================================================================
    // Possible callback methods
    // ========================================================================

    function onProxyCreationDone() {
        // Handle parent visibility if need be
        if(pipelineDataModel.source.hasOwnProperty('hints') && pipelineDataModel.source.hints.hasOwnProperty('replaceInput')) {
            var replaceStrategie = pipelineDataModel.source.hints.replaceInput;
            if(replaceStrategie > 0) {
                // We are not handling the case with 2 but we could eventually
                // For 2 we need to check representation Opacity and Representation property state
                startWorking();
                session.call('pv.proxy.manager.update', [[{ 'name': 'Visibility', 'value': 0, 'id': getProxyMetaData(pipelineDataModel.metadata.parent).rep } ]]).then(invalidatePipeline, invalidatePipeline);
            }
        }
    }

    // ========================================================================
    // Busy feedback
    // ========================================================================

    function startWorking() {
        busyCount++;
        if(busyCount === 1) {
            notBusyElement.hide();
            busyElement.show();
        }
    }

    // ------------------------------------------------------------------------

    function workDone() {
        busyCount--;
        if(busyCount === 0) {
            busyElement.hide();
            notBusyElement.show();
        }
    }

    // ========================================================================
    // Helper methods
    // ========================================================================

    function updatePaletteNames() {
        session.call('pv.color.manager.list.preset', []).then(function(list){
            paletteNameList = list;
        });
    }

    // ------------------------------------------------------------------------

    function activeProxy(newActiveProxy) {
        if(newActiveProxy) {
            pipeline.data('pipeline').active_proxy = newActiveProxy;
        }
        return pipeline.data('pipeline').active_proxy;
    }

    // ------------------------------------------------------------------------

    function getProxyMetaData(id) {
        var list = pipelineDataModel.sources,
            count = list.length;

        while(count--) {
            if(list[count].id === id) {
                return list[count];
            }
        }
        return null;
    }

    // ------------------------------------------------------------------------

    function loadProxy(proxy_id, name) {
        if(session) {
            startWorking();
            session.call("pv.proxy.manager.get", [proxy_id]).then(function(proxy){
                workDone();
                pipelineDataModel[name] = proxy;
                onNewProxyLoaded();
            }, error);
        }
    }

    // ------------------------------------------------------------------------

    function activePipelineInspector() {
        $('.inspector-selector[data-type="pipeline"]').trigger('click');
        $('.inspector-container').scrollTop(0);
    }

    // ------------------------------------------------------------------------

    function addFixHeightBehavior() {
        $(window).resize(function() {
            var windowHeight = $(window).height();
            $('.fix-height').each(function(){
                var delta = this.parentNode.getBoundingClientRect().top,
                    me = $(this);

                me.css('height', (windowHeight - delta) + 'px');
                if(me.hasClass('shift-top')) {
                    me.css('top', delta + 'px');
                }
                updateView();
            });
        }).trigger('resize');
    }

    // ------------------------------------------------------------------------

    function addDefaultButtonsBehavior() {
        var inspectorContainer = $('.inspector-container'),
            inspectors = $('.inspector');

        // Toggle inspector visibility
        $('.toggle-inspector').click(function(){
            if(inspectorContainer.is(':visible')) {
                inspectorContainer.hide();
            } else {
                inspectorContainer.show();
            }
        });

        // Toggle inspector visibility
        $('.inspector-selector').click(function(){
            var me = $(this),
                activeType = me.attr('data-type');

            inspectors.hide();
            $('.inspector[data-type="TYPE"]'.replace(/TYPE/g, activeType)).show();
            inspectorContainer.show();

            if(activeType === 'pipeline') {
                inspectorContainer.scrollTop(0);
            }
        });

        // Reset camera
        $('.reset-camera').click(resetCamera);
    }

    // ------------------------------------------------------------------------

    function addScrollBehavior() {
        $('.inspector-container').scroll(function (event) {
            var y = $(this).scrollTop();
                reservedHeight = 0;
            if ($('.pv-pipeline').is(':visible') === true) {
                reservedHeight = $('.pv-pipeline').height();
            } else if ($('.pv-preferences').is(':visible') === true) {
                reservedHeight = $('.pv-preferences').height();
            }
            if (y >= reservedHeight) {
                $('.pv-editor-bar').addClass('pv-fixed').css('top', (y-reservedHeight)+'px');
            } else {
                $('.pv-editor-bar').removeClass('pv-fixed');
            }
        });
    }

    // ========================================================================
    // Preference Panel (Renderer Type...)
    // ========================================================================

    function addPreferencePanelBehavior() {
        $('.pv-update-viewport').bind('change', function(){
            var me = $(this),
                propName = me.attr('data-property-name'),
                value = me.hasClass('checkbox') ? (me.prop('checked') ? 1 : 0) : me.val();
            if(propName === 'ActiveRendererType') {
                viewport.setActiveRenderer(value);
                viewport.invalidateScene();
            } else if(propName === 'Stats') {
                viewport.showStatistics(value === 1);
            }
        });
    }

    // ========================================================================
    // Time / Animation feedback
    // ========================================================================

    function addTimeAnimationButtonsBehavior() {
        // Action management
        $('.vcr-play').click(onTimeAnimationPlay);
        $('.vcr-stop').hide().click(onTimeAnimationStop);
        $('.vcr-action').click(onTimeAnimationAction);

        // Handle visibility toggle
        $('.toggle-time-toolbar').click(function(){
            var toolbar = $('.vcr-control'),
                me = $(this);
            if(me.hasClass('pv-text-red')) {
                $(this).removeClass('pv-text-red');
                toolbar.hide();
            } else {
                toolbar.show();
                $(this).addClass('pv-text-red');
            }
        });

        // Update default state
        $('.vcr-control').hide();
    }

    // ------------------------------------------------------------------------

    function onTimeAnimationAction() {
        startWorking()
        session.call('pv.vcr.action', [ $(this).attr('data-action')]).then(function(timeValue){
            $('.time-value').val(timeValue);
            updateView();
            workDone();
        }, error);
    }

    // ------------------------------------------------------------------------

    function onTimeAnimationPlay() {
        $('.vcr-play').hide();
        $('.vcr-stop').show();
        vcrPlayStatus = true;
        runTimeAnimationLoop();
    }

    // ------------------------------------------------------------------------

    function onTimeAnimationStop() {
        $('.vcr-play').show();
        $('.vcr-stop').hide();
        vcrPlayStatus = false;
    }

    // ------------------------------------------------------------------------

    function runTimeAnimationLoop() {
        if(vcrPlayStatus) {
            session.call('pv.vcr.action', ['next']).then(function(timeValue){
                $('.time-value').val(timeValue);
                updateView();
                setTimeout(runTimeAnimationLoop, 50);
            });
        }
    }

    // ========================================================================
    // Proxy Creation management (sources + filters)
    // ========================================================================

    function createCreationView(selector, category) {
        var panelContainer = $(selector);

        function onDataCreation(action) {
            if(session) {
                startWorking()
                session.call('pv.proxy.manager.create', [action.value, activeProxy()]).then(function(arg){
                    pipelineLoadedCallBack = onProxyCreationDone;
                    invalidatePipeline(arg);
                    activePipelineInspector();
                }, error);
            }
        }

        startWorking();
        session.call('pv.proxy.manager.available',[category]).then(function(list){
            var actionList = [],
                size = list.length;

            list.sort();
            for(var idx = 0; idx < size; ++idx) {
                actionList.push({ id: list[idx], label: list[idx], value: list[idx]});
            }
            panelContainer.actionList({actions:actionList});
            workDone();
        }, error);

        panelContainer.bind('action', onDataCreation);
    }

    // ========================================================================
    // Data Information Panel
    // ========================================================================

    function createDataInformationPanel(dataInfoSelector) {
        infoManager = $(dataInfoSelector).empty();
    }

    // ------------------------------------------------------------------------

    function updateDataInformationPanel(data) {
        if(data) {
            infoManager.pvDataInformation(data);
        } else {
            infoManager.empty();
        }
    }

    // ========================================================================
    // ViewPort management (active + visibility)
    // ========================================================================

    function createViewportView(viewportSelector) {
        $(viewportSelector).empty();
        viewport = vtkWeb.createViewport({session: session});
        viewport.bind(viewportSelector);
    }

    // ------------------------------------------------------------------------

    function updateView() {
        if(viewport) {
            viewport.invalidateScene();
        }
    }

    // ------------------------------------------------------------------------

    function resetCamera() {
        if(viewport) {
            viewport.resetCamera();
        }
    }


    // ========================================================================
    // Pipeline management (active + visibility)
    // ========================================================================

    function createPipelineManagerView(pipelineSelector) {
        pipeline = $(pipelineSelector);
        pipeline.pipelineSVG({session: session});
        pipeline.bind('pipeline-visibility-change', onProxyVisibilityChange);
        pipeline.bind('pipeline-data-change', onPipelineDataChange);
        pipeline.trigger('pipeline-reload');
    }

    // ------------------------------------------------------------------------

    function onProxyVisibilityChange(proxy) {
        updateView();
    }

   // ------------------------------------------------------------------------

    function onPipelineDataChange(event) {
        // { active: active_proxy_id, view: view_id, sources: proxy_list }

        // Update data model
        pipelineDataModel.sources = event.sources;

        // Handle the new active proxy
        if(event.active === '0') {
            $('.need-input-source').hide();
            proxyEditor.empty();
            updateView();
        } else {
            $('.need-input-source').show();
            pipelineDataModel.metadata = getProxyMetaData(event.active);
            pipelineDataModel.source = null;
            pipelineDataModel.representation = null;
            if(pipelineDataModel.metadata) {
                loadProxy(pipelineDataModel.metadata.id, 'source');
                loadProxy(pipelineDataModel.metadata.rep, 'representation');
                if(pipelineDataModel.view === null) {
                    loadProxy(event.view, 'view');
                }
            }
        }
    }

    // ========================================================================
    // Global settings management
    // ========================================================================

    function reloadSettingsEditor() {
        startWorking();
        settingsEditor.empty();

        if (rvSettingsProxyId !== null) {
            session.call('pv.proxy.manager.get', [rvSettingsProxyId]).then(function(rvSettingsProps) {

                var props = [].concat(
                    "+RenderViewSettings", rvSettingsProps.properties, '_RenderViewSettings'
                    ),
                ui = [].concat(
                    "+RenderViewSettings", rvSettingsProps.ui, '_RenderViewSettings'
                    );

                settingsEditor.proxyEditor("Global Settings", false, 0, props, ui, [], [], {});

                workDone();
            }, function(rvSettingsErr) {
                console.log("Failed to get the RenderViewSettings proxy properties:");
                console.log(rvSettingsErr);
                workDone();
            });
        }
    }

    function createGlobalSettingsPanel(settingsSelector) {
        startWorking();
        settingsEditor = $(settingsSelector);

        // Get the proxy id of the RenderViewSettings proxy
        session.call('pv.proxy.manager.find.id', ['settings', 'RenderViewSettings']).then(function(proxyId) {
            rvSettingsProxyId = proxyId;
            workDone();
            reloadSettingsEditor();
        }, function(proxyIdErr) {
            console.log('Error getting RenderViewSettings proxy id:');
            console.log(proxyIdErr);
            workDone();
        });

        // To apply render view settings, first update the proxy, then reload it
        settingsEditor.bind('apply', function(evt) {
            startWorking();
            session.call('pv.proxy.manager.update', [evt.properties]).then(function(updateResult) {
                console.log("Successfully updated RenderViewSettings proxy:")
                console.log(updateResult);
                viewport.invalidateScene();
                reloadSettingsEditor();
                workDone();
            }, function(updateError) {
                console.log("Failed to update RenderViewSettings proxy:")
                console.log(updateError);
                workDone();
            });
        });
    }

    // ========================================================================
    // Proxy Editor management (update + delete)
    // ========================================================================

    function createProxyEditorView(proxyEditorSelector) {
        proxyEditor = $(proxyEditorSelector);
        proxyEditor.bind('delete-proxy', onProxyDelete);
        proxyEditor.bind('apply', onProxyApply);
        proxyEditor.bind('scalarbar-visibility', onScalarBarVisibility);
        proxyEditor.bind('rescale-transfer-function', onRescaleTransferFunction);
        proxyEditor.bind('update-scalar-opacity-function', onUpdateOpacityPoints);
        proxyEditor.bind('store-scalar-opacity-parameters', onStoreOpacityParameters);
        proxyEditor.bind('initialize-scalar-opacity-widget', onInitializeScalarOpacityWidget);
        proxyEditor.bind('request-scalar-range', onRequestScalarRange);
        proxyEditor.bind('push-new-surface-opacity', onSurfaceOpacityChanged);
    }

    // ------------------------------------------------------------------------

    function onProxyDelete(event) {
        startWorking();
        session.call('pv.proxy.manager.delete', [event.id]).then(invalidatePipeline, invalidatePipeline);
    }

    // ------------------------------------------------------------------------

    function onProxyApply(event) {
        startWorking();
        session.call('pv.proxy.manager.update', [event.properties]).then(invalidatePipeline, invalidatePipeline);

        // Args: representation, colorMode, arrayLocation='POINTS', arrayName='', vectorMode='Magnitude', vectorComponent = 0, rescale=False
        var args = [].concat(event.colorBy.representation, event.colorBy.mode, event.colorBy.array, event.colorBy.component);
        startWorking();
        session.call('pv.color.manager.color.by', args).then(invalidatePipeline, error);
        // Update palette ?
        if(event.colorBy.palette) {
            startWorking();
            session.call('pv.color.manager.select.preset', [ event.colorBy.representation, event.colorBy.palette ]).then(invalidatePipeline, error);
        }
    }

    // ------------------------------------------------------------------------

    function onScalarBarVisibility(event) {
        startWorking();
        var visibilityMap = {};
        visibilityMap[event.id] = event.visible;
        session.call('pv.color.manager.scalarbar.visibility.set', [visibilityMap]).then(invalidatePipeline, invalidatePipeline);
    }

    // ------------------------------------------------------------------------

    function onUpdateOpacityPoints(event) {
        var colorBy = event.colorBy;
        if (colorBy.array.length >= 2 && colorBy.array[1] !== '') {
            var args = [colorBy.array[1], event.points];
            startWorking();
            session.call('pv.color.manager.opacity.points.set', args).then(function(successResult) {
                viewport.invalidateScene();
                workDone();
            }, workDone);
        }
    }

    // ------------------------------------------------------------------------

    function onSurfaceOpacityChanged(event) {
        var colorBy = event.colorBy;
        if (colorBy.array.length >= 2 && colorBy.array[1] !== '') {
            var args = [colorBy.representation, (event.opacity === true ? 1 : 0)];
            startWorking();
            session.call('pv.color.manager.surface.opacity.set', args).then(function(successResult) {
                viewport.invalidateScene();
                workDone();
            }, workDone);
        }
    }

    // ------------------------------------------------------------------------

    function onStoreOpacityParameters(event) {
        var colorArray = event.colorBy.array;
        if (colorArray.length >= 2 && colorArray[1] !== '') {
            var storeKey = colorArray[1] + ":opacityParameters";
            var args = [storeKey, event.parameters];
            startWorking();
            session.call('pv.keyvaluepair.store', args).then(workDone, workDone);
        }
    }

    // ------------------------------------------------------------------------

    function onRequestScalarRange(event) {
        var proxyId = event.proxyId;
        startWorking();
        session.call('pv.color.manager.scalar.range.get', [proxyId]).then(function(curScalarRange) {
            proxyEditor.trigger({
                'type': 'update-scalar-range-values',
                'min': curScalarRange.min,
                'max': curScalarRange.max
            });
            workDone();
        }, workDone);
    }

    // ------------------------------------------------------------------------

    function onRescaleTransferFunction(event) {
        startWorking();
        var options = { proxyId: event.id, type: event.mode };
        if(event.mode === 'custom') {
            options.min = event.min;
            options.max = event.max;
        }
        session.call('pv.color.manager.rescale.transfer.function', [options]).then(function(successResult) {
            if (successResult['success'] === true) {
                viewport.invalidateScene();
                proxyEditor.trigger({
                    'type': 'update-scalar-range-values',
                    'min': successResult.range.min,
                    'max': successResult.range.max
                });
            }
            workDone();
        }, workDone);
    }

    // ------------------------------------------------------------------------

    function extractRepresentation(list) {
        var count = list.length;
        while(count--) {
            if(list[count].name === 'Representation') {
                return [list[count]];
            }
        }
        return [];
    }

    // ------------------------------------------------------------------------

    function onNewProxyLoaded() {
        if(pipelineDataModel.metadata && pipelineDataModel.source && pipelineDataModel.representation && pipelineDataModel.view) {
            var props = [].concat(
                    "+Color Management",
                    extractRepresentation(pipelineDataModel.representation.properties),
                    "ColorByPanel",
                    "_Color Management",
                    "+Source", pipelineDataModel.source.properties, '_Source',
                    "-Representation", pipelineDataModel.representation.properties, '_Representation',
                    "-View", pipelineDataModel.view.properties, "_View"
                    ),
                ui = [].concat(
                    "+Color Management",
                    extractRepresentation(pipelineDataModel.representation.ui),
                    "ColorByPanel",
                    "_Color Management",
                    "+Source", pipelineDataModel.source.ui, '_Source',
                    "-Representation", pipelineDataModel.representation.ui, '_Representation',
                    "-View", pipelineDataModel.view.ui, "_View"
                    );


            try {
                proxyEditor.proxyEditor(pipelineDataModel.metadata.name,
                                        pipelineDataModel.metadata.leaf,
                                        pipelineDataModel.metadata.id,
                                        props,
                                        ui,
                                        pipelineDataModel.source.data.arrays,
                                        paletteNameList,
                                        pipelineDataModel.representation.colorBy);
                $('.inspector-container').scrollTop(0);
            } catch(err) {
                console.log(err);
            }
            // Update information section
            updateDataInformationPanel(pipelineDataModel.source.data);

            // Handle callback if any
            if(pipelineLoadedCallBack) {
                pipelineLoadedCallBack();
                pipelineLoadedCallBack = null;
            }

            // Handle automatic reset camera
            if(pipelineDataModel.sources.length === 1) {
                resetCamera();
            } else {
                updateView();
            }
        }
    }

    // ========================================================================
    // Opacity editor widget creation
    // ========================================================================
    function onInitializeScalarOpacityWidget(event) {
        var container = event.container,
            colorArray = event.colorBy.array,
            needParams = [ 'currentPointSet', 'surfaceOpacityEnabled' ],
            initOptions = {
                'buttonsPosition': 'top',
                'topMargin': 10,
                'rightMargin': 15,
                'bottomMargin': 10,
                'leftMargin': 15
            };

        function gotInitParam(paramName) {
            needParams.splice(needParams.indexOf(paramName), 1);
            if (needParams.length === 0) {
                container.opacityEditor(initOptions);
            }
        }

        if (colorArray.length >= 2 && colorArray[1] !== '') {

            var retrieveKey = colorArray[1] + ":opacityParameters";
            session.call('pv.keyvaluepair.retrieve', [retrieveKey]).then(function(result) {
                if (result !== null) {
                    initOptions.gaussiansList = result.gaussianPoints;
                    initOptions.linearPoints = result.linearPoints;
                    initOptions.gaussianMode = result.gaussianMode;
                    initOptions.interactiveMode = result.interactiveMode;
                }
                gotInitParam('currentPointSet');
                workDone();
            }, workDone);

            var representation = event.colorBy.representation;
            session.call('pv.color.manager.surface.opacity.get', [representation]).then(function(result) {
                if (result !== null) {
                    initOptions.surfaceOpacityEnabled = (result === 1 ? true : false);
                }
                gotInitParam('surfaceOpacityEnabled');
                workDone();
            }, workDone);

        } else {
            console.log("WARNING: Initializing the opacity editor while not coloring by an array.");
            container.opacityEditor(initOptions);
        }
    }

    // ========================================================================
    // File management (browse + open)
    // ========================================================================

    function createFileManagerView(fileSelector) {
        var fileManagerContainer = $(fileSelector);

        function onFileAction(event) {
            var action = event.action,
                basePath = event.path,
                files = event.files;

            if(action === 'directory' || action === 'path') {
                if(basePath.length === 0) {
                    startWorking();
                    session.call('file.server.directory.list', ['.']).then(function(dirContent){
                        fileManagerContainer.fileBrowser2(dirContent);
                        workDone();
                    }, error);
                } else {
                    var fullPath = [].concat(basePath.split('/'), files).join('/');
                    startWorking();
                    session.call('file.server.directory.list', [fullPath]).then(function(dirContent){
                        fileManagerContainer.fileBrowser2(dirContent);
                        workDone();
                    }, error);
                }
            } else if(action === 'file') {
                var fullpathFileList = [],
                    list = files.split(','),
                    count = list.length;
                for(var i = 0; i < count; ++i) {
                    fullpathFileList.push([].concat(basePath.split('/').slice(1), list[i]).join('/'));
                }
                startWorking();
                session.call("pv.proxy.manager.create.reader", [fullpathFileList]).then(invalidatePipeline, invalidatePipeline);
                activePipelineInspector();
            }
        }

        // Get initial content and handle events internally
        startWorking();
        session.call('file.server.directory.list', ['.']).then(function(dirContent) {
            fileManagerContainer.fileBrowser2(dirContent);
            fileManagerContainer.bind('file-action', onFileAction);
            workDone();
        }, error);
    }

    // ========================================================================
    // Main - Visualizer Setup
    // ========================================================================

    function initializeVisualizer(session_, viewportSelector, pipelineSelector, proxyEditorSelector, fileSelector, sourceSelector, filterSelector, dataInfoSelector, settingsSelector) {
        session = session_;

        // Initialize data and DOM behavior
        updatePaletteNames();
        addScrollBehavior();
        addDefaultButtonsBehavior();
        addFixHeightBehavior();
        addTimeAnimationButtonsBehavior();
        addPreferencePanelBehavior();

        // Create panels
        createFileManagerView(fileSelector);
        createCreationView(sourceSelector, 'sources');
        createCreationView(filterSelector, 'filters');
        createViewportView(viewportSelector);
        createPipelineManagerView(pipelineSelector);
        createProxyEditorView(proxyEditorSelector);
        createDataInformationPanel(dataInfoSelector);
        createGlobalSettingsPanel(settingsSelector);

        // Set initial state
        $('.need-input-source').hide();
        proxyEditor.empty();
        activePipelineInspector();
    };

    // ----------------------------------------------------------------------
    // Init vtkWeb module if needed
    // ----------------------------------------------------------------------
    if (GLOBAL.hasOwnProperty("pv")) {
        module = GLOBAL.pv || {};
    } else {
        GLOBAL.pv = module;
    }

    // Expose some methods to pv namespace
    module.initializeVisualizer = initializeVisualizer;

}(window, jQuery));