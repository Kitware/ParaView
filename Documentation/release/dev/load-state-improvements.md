# Improvements to 'LoadState' Python API

When loading pvsm state files in Python using `LoadState` function, it was
tricky to provide arguments to override data files used in the state file.
The `LoadState` function has now been modified to enable users to specify
filenames to override using a Python dictionary. The Python trace captures
this new way of loading state files. Invocations of this function using
previously used arguments is still supported and will continue to work.

Some examples:

    # use data files under a custom directory
    LoadState(".....pvsm",
              data_directory="/...",
              restrict_to_data_directory=True)

    # explicitly override files
    LoadState(".....pvsm",
        filenames=[\
            {
                'name' : 'can.ex2',
                'FileName' : '/..../disk_out_ref.ex2',
            },
            {
                'name' : 'timeseries',
                'FileName' : [ '/..../sample_0.vtp',
                               '/..../sample_1.vtp',
                               '/..../sample_2.vtp',
                               '/..../sample_3.vtp',
                               '/..../sample_4.vtp',
                               '/..../sample_5.vtp',
                               '/..../sample_6.vtp',
                               '/..../sample_7.vtp',
                               '/..../sample_8.vtp',
                               '/..../sample_9.vtp',
                               '/..../sample_10.vtp']
            },
        ])
