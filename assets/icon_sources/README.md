# PWDer icon drawing mechanism
PWDer stores its icons in program memory under a pwder_style::icon32 struct (see `/include/icons.h`).
The struct contains following data:

## .bitmap[3072] (uint8_t)
This is a byte array containing RGB data of the icon, pixel by pixel.
`32 x 32` canvas * `3` bytes for each color = `3072` bytes.

## .mask[1024]
This is a bool array containing a mask, telling the push_icon function whether the pixel should be drawn or not.
It is useful when a transparent background has to be drawn.

* If the value is set to `true`, the pixel will be masked (will not be drawn).
* If the value is set to `false`, the pixel will not be masked (will be drawn).

## Using your own icons
The requirement is that the icon size is `32 x 32` pixels.

1. Using your favorite raster graphic editing program, create a 24-bit `32 x 32` BMP file: the colored icon that will be displayed.

2. Create another, monochrome `32 x 32` BMP file: the mask that will hide the background or anything that should be transparent.
    * Fill masked area with white pixels and unmasked area with black pixels.
    
3. Write a script to convert the RGB BMP to C++ array:
    * Structure:
        ```cpp
            /* Red byte */, /* Green byte */, /* Blue byte */, /* Red byte */, /* Green byte */, /* ... */
        ```
    * Example:
        ```cpp
            0x00, 0x00, 0x80, 0xFF, 0xFF, 0xFF, /* ... */
        ```
4. Convert the mask BMP to C++ array, converting black pixels to `false` and white ones to `true`:
    * Example:
        ```cpp
            true, true, false, false, false, true, false, /* ... */
        ```

*(I will include the scripts here as well soon)*

5. Replace data in `icons.cpp`
    ```cpp
        const pwder_style::icon32 icon_name PROGMEM = {
            .bitmap = {
                0x00, 0x00, 0x80, 0xFF, 0xFF, 0xFF, /* ... */
            },
            .mask = {
                true, true, false, false, false, true, false, /* ... */
            },
        };
    ```

6. Recompile, and now you have a custom icon.
