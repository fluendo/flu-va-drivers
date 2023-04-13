# flu-va-drivers-vdpau

This package includes drivers for _libva_ that act as a translation layer from
VA-API to VDPAU.

## Requirements

The following dependencies are needed:

  - libva >= 2.10.0 (VA-API >= 1.10)

## How to use

Ensure to set the environment variable `LIBVA_DRIVER_NAME=flu_va_drivers_vdpau`
and `LIBVA_DRIVERS_PATH` to point to the path of where the
*flu_va_drivers_vdpau_drv_video.so* file is located.

### Google Chrome (Chromium)

In order to get Google Chrome using this project, you have to run Google Chrome
as following:
```sh
LIBVA_DRIVER_NAME=flu_va_drivers_vdpau \
LIBVA_DRIVERS_PATH=$(pkg-config --variable=prefix libva) \
google-chrome --enable-features=VaapiVideoDecoder --ignore-gpu-blocklist --use-gl=desktop
```

Supported Google Chrome version is up to 109.0.5414.119. Note, that from version
98.0.4758.80 the following argument should be added: `--disable-features=UseChromeOSDirectVideoDecoder`

#### Chromium

In the case of Chromium, the same arguments have to be provided, but also you
have to ensure to have compiled Chromium with vaapi support, adding the
following to your `gn.args` Chromium configuration file:

```
use_vaapi=true
proprietary_codecs=true
ffmpeg_branding="Chrome"
```

For more information, see [the official documentation for VA-API support in
Chromium](https://chromium.googlesource.com/chromium/src/+/master/docs/gpu/vaapi.md)

#### Test
Download and install [Google Chrome version 109.0.5414.119](https://dl.google.com/linux/chrome/deb/pool/main/g/google-chrome-stable/google-chrome-stable_109.0.5414.119-1_amd64.deb).

Generate a test video file encoded as H.264:
```
ffmpeg -f lavfi -i testsrc=duration=10:size=1280x720:rate=30 -pix_fmt yuv420p -vcodec h264 /tmp/test420.mp4
```

Run Google Chrome with the following environment variables and options:
```
LIBVA_DRIVER_NAME=flu_va_drivers_vdpau \
LIBVA_DRIVERS_PATH=$PWD/builddir/src/ \
google-chrome-stable --use-gl=desktop --enable-features=VaapiVideoDecoder \
    --disable-features=UseChromeOSDirectVideoDecoder /tmp/test420.mp4 \
    --auto-open-devtools-for-tabs
```

To check that Google Chrome is using VA-API, do the following:
1. Input the following keys combination: <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>I</kbd>
2. In the right panel, go to three dots menu button and navigate to "More Tools > Media".
3. In the right panel (with the "FrameTitle test420.mp4" item selected under
"Players" section), in the "Video Decoder" section, the "Decoder name" field
should be set "VDAVideoDecoder". If it is set to FFmpegVideoDecoder or something else,
then Google Chrome is not using VA-API.
