add_test(
    NAME
        TestServerManagerRemoteConnection
    COMMAND
        $<TARGET_FILE:ParaView::smTestDriver>
        --server $<TARGET_FILE:ParaView::pvserver>
        --client $<TARGET_FILE:ServerManagerRemoteConnectionExample>
        --test
)
