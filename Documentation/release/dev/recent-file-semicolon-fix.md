## Handle semicolon and hash in Recent Files menu

Fix a bug which would crash ParaView on startup if a file
was opened with a semicolon ";" in the filename. The Recent
Files menu and saved settings uses ";" as a record separator,
and hash "#" as a server name indicator. Encode their use in
filenames.
