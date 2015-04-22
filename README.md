# BlurPat

_The project is nothing serious. It is created for temporary use for specific problem._

This is a simple tool to blur some pattern on an image.

# Usage

The following command outputs usage information
```
blurpat -h
```

## Example

The following searches for region similar to `logo.png` on `org.jpg`. If found,
applies Gaussian blur operation to that region plus 120 pixels to the right plus
120 pixels to the left using:

* 15x15px kernel;
* 500px wide line at the bottom of the image as the region of interest;
* 45 as noise suppression threshold value.

Result is written to `out.jpg` file.

```
blurpat -vv -k 15 -m 0,120,0,120 -r -0,-500 -t 45 -i org.jpg -o out.jpg logo.png
```

# Author

Ruslan Osmanov <rrosmanov@gmail.com>
