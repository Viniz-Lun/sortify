Dependency needed: inotify-tools, extremely lightweight.

Compile with a c compiler and run it in foreground or background to have it watch your ~/Download folder.

It will transfer files DOWNLOADED into that directory into a subdirectory (Download/images for images and Download/pdfs for .pdf files).

For now it only watches the home directory of user vin because i forgot to make it generic.
