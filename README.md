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

This step can be omitted following the steps in on "How to use" section.
```
sudo ninja install
```

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

### Testing on Google Chrome (Chromium)

Refer to [data/doc/flu-va-drivers/README.md](data/doc/flu-va-drivers/README.md).

### Other similar projects

- https://github.com/freedesktop/vdpau-driver
- https://github.com/elFarto/nvidia-vaapi-driver
