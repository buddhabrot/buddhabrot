#!/bin/bash
./MacOSX/Buddhabrot/build/Buddhabrot/Build/Products/Debug/brot $@
find . -name "out.png" -exec open {} + 
