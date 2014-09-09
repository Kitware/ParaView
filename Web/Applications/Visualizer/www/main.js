(function (GLOBAL, $) {
    var session,
        viewport,
        pipeline,
        proxyEditor,
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
            var y = $(this).scrollTop(),
                pipelineHeight = $('.pv-pipeline').height();
            if (y >= pipelineHeight) {
                $('.pv-editor-bar').addClass('pv-fixed').css('top', (y-pipelineHeight)+'px');
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
    // Proxy Editor management (update + delete)
    // ========================================================================

    function createProxyEditorView(proxyEditorSelector) {
        proxyEditor = $(proxyEditorSelector);
        proxyEditor.bind('delete-proxy', onProxyDelete);
        proxyEditor.bind('apply', onProxyApply);
        proxyEditor.bind('scalarbar-visibility', onScalarBarVisibility);
        proxyEditor.bind('rescale-transfer-function', onRescaleTransferFunction);
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

    function onRescaleTransferFunction(event) {
        startWorking();
        var options = { proxyId: event.id, type: event.mode };
        if(event.mode === 'custom') {
            options.min = event.min;
            options.max = event.max;
        }
        session.call('pv.color.manager.rescale.transfer.function', [options]).then(invalidatePipeline, invalidatePipeline);
    }

    // ------------------------------------------------------------------------

    function onNewProxyLoaded() {
        if(pipelineDataModel.metadata && pipelineDataModel.source && pipelineDataModel.representation && pipelineDataModel.view) {
            var props = [].concat(
                    "+Source", pipelineDataModel.source.properties, '_Source',
                    "-Representation", "ColorByPanel", pipelineDataModel.representation.properties, '_Representation',
                    "-View", pipelineDataModel.view.properties, "_View"
                    ),
                ui = [].concat(
                    "+Source", pipelineDataModel.source.ui, '_Source',
                    "-Representation", "ColorByPanel", pipelineDataModel.representation.ui, '_Representation',
                    "-View", pipelineDataModel.view.ui, "_View"
                    );


            try {
                proxyEditor.proxyEditor(pipelineDataModel.metadata.name, pipelineDataModel.metadata.leaf, pipelineDataModel.metadata.id, props, ui, pipelineDataModel.source.data.arrays, paletteNameList, pipelineDataModel.representation.colorBy);
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

    function initializeVisualizer(session_, viewportSelector, pipelineSelector, proxyEditorSelector, fileSelector, sourceSelector, filterSelector, dataInfoSelector) {
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