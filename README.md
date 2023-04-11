# flu-va-drivers

Drivers for _libva_ that act as a translation layer to VA-API from:

 - VDPAU (default)
 - ...

In a future more drivers could be added.

# Requirements

Refer to [data/doc/flu-va-drivers/README.md](data/doc/flu-va-drivers/README.md).

# How to compile

```sh
mkdir builddir
meson builddir
ninja -C builddir
# or: meson compile -C builddir
```

# How to install

Refer to [data/doc/flu-va-drivers/README.md](data/doc/flu-va-drivers/README.md).

# How to use 

Refer to [data/doc/flu-va-drivers/README.md](data/doc/flu-va-drivers/README.md).

# Supported formats

The following decoders are supported:
- H264

The following profiles are supported:
- VAProfileH264ConstrainedBaseline
- VAProfileH264Main
- VAProfileH264High

The following chroma subsamplings are supported:
- VA_RT_FORMAT_YUV420

# Release process

1. First, ensure that Github Actions build workflow on last commit is on green status.
2. Submit a PR with following changes: change version in meson.build to the desired one, e.g. 0.0.1
3. Wait for that PR to be merged.
4. Tag the resulting commit as the desired release version, e.g. 0.0.1
5. Submit a PR with the following changes: change version in meson.build to the next desired version, suffixed by `-dev`, e.g. 0.0.2-dev.

# Testing

## VDPAU

### FFMPEG
The way we're testing consist on try to decode using `ffmpeg` with this
driver and checking the libva trace and/or debugging it with `gdb`. Here is an
example script to do it:

```sh
#gdb="gdb --args"
url='https://www.itu.int/wftp3/av-arch/jvt-site/draft_conformance/AVCv1/CVWP1_TOSHIBA_E.zip'
test_file="CVWP1_TOSHIBA_E.264"

[ -f "$test_file" ] || wget "$url" -qO - | zcat > "$test_file"
rm -fv ./deleteme.out
rm -fv ./libva_ffmpeg*

DISPLAY=:0 \
LIBVA_TRACE=./libva_ffmpeg \
LIBVA_DRIVER_NAME=flu_va_drivers_vdpau \
LIBVA_DRIVERS_PATH=$PWD/builddir/src/ \
$gdb ffmpeg \
	-hwaccel vaapi \
	-threads 1 \
	-init_hw_device "vaapi=vaapi0:,connection_type=x11" \
	-i "$test_file" \
	-vf format=pix_fmts=yuv420p \
	-f rawvideo \
	./deleteme.out
```

### Chrome
Supported Google Chrome version is 97.0.4692.99. To fetch this version you can
use the following [link](https://dl.google.com/linux/chrome/deb/pool/main/g/google-chrome-stable/google-chrome-stable_97.0.4692.99-1_amd64.deb).

The steps to test Google Chrome with flu-va-drivers are the following:

First, generate a test video file encoded as H.264:
```
ffmpeg -f lavfi -i testsrc=duration=10:size=1280x720:rate=30 -pix_fmt yuv420p /tmp/test420.mp4
```

Run Google Chrome with the following environment variables and options:
```
LIBVA_DRIVER_NAME=flu_va_drivers_vdpau \
LIBVA_DRIVERS_PATH=$PWD/builddir/src/ \
google-chrome-stable --use-gl=desktop --enable-features=VaapiVideoDecoder \
    --disable-features=UserChromeOSDirectVideoEncoder /tmp/test420.mp4 \
    --auto-open-devtools-for-tabs
```

To check that Google Chrome is using VA-API, do the following:
1. Input the following keys combination: <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>I</kbd>
2. In the right panel, go to three dots menu button and navigate to "More Tools > Media".
3. In the right panel (with the "FrameTitle test420.mp4" item selected under
"Players" section), in the "Video Decoder" section, the "Decoder name" field
should be set "VDAVideoDecoder". If it is set to FFmpegVideoDecoder or something else,
then Google Chrome is not using VA-API.

### Other similar projects

- https://github.com/freedesktop/vdpau-driver
- https://github.com/elFarto/nvidia-vaapi-driver
