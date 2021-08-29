# Buddhabrot generator.

![Rendering](buddhabrot.png?raw=true "A Rendering of a buddhabrot - post-processed a little to separate the depth of iterations into R/G/B channels")

## Note: The above preview is being minted as an NFT by me.

Usage:
buddhabrot size depth filepath [mandelbrot_factor]

Calculates a buddhabrot fractal with size {size} in pixels, with iterative depth {depth}, and outputs the resulting image files (for the mandelbrot and the buddhabrot) to {filepath}.
Optionally, you can specify a factor {mandelbrot_factor}, which will multiply the iterative depth for calculating the mandelbrot fractal. This way you can have a more detailed mandelbrot to use as a basis to calculate your buddhabrot (this is advised, since mandelbrot creation is pretty cheap).

Needed for image creation are libpng and libgd (any version will do). I'll make a standalone image writer someday, or you can add your own.

## Dependencies #

hpalib (http://www.nongnu.org/hpalib/)
Note that if you build hpalib, it should use the same XDIM settings as any cache files you're using, or the values will not de/serialize correctly.

For MacOSX, CImg will require X11. You will need to install from the XQuartz project if you're running 10.8 or later.
