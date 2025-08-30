Dependency needed: inotify-tools, lightweight api for tracking events in the filesystem.

Compile with a c compiler and run it in foreground or background to have it watch your ~/Download folder.

It will transfer files DOWNLOADED in that directory, from a web browser, into a subdirectory (Download/images for images and Download/pdfs for .pdf files).

To determine when a file has been downloaded into the directory the behaviour firefox employs when downloading files has been analyzed, so it might be specific to that platform.
