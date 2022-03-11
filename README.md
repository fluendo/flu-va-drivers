# flu-va-drivers

Drivers for _libva_ that act as a translation layer to VAAPI from:

 - VDPAU (default)
 - ...

In a future more drivers could be added.

# Requirements

The following dependencies are needed:

  - libva >= 2.10.0 (VA-API >= 1.10)

# How to compile

```sh
mkdir builddir
meson builddir
ninja -C builddir
# or: meson compile -C builddir
```

# How to install

You can copy the generated *flu_va_drivers_vdpau_drv_video.so* file
(in *builddir/src*) to your libva driver directory (given by
`pkg-config --variable=driverdir libva`) or do the following:

```sh
ninja -C builddir install
```

If you have a distributed *.deb* file it will be as simple as (for example for
the vdpau driver):
```sh
sudo apt install flu-va-drivers-vdpau_x.y.z.62155e4d-1_amd64.deb
```
# How to use 

Then ensure to set the environment variable `LIBVA_DRIVER_NAME=flu_va_drivers_vdpau`
and `LIBVA_DRIVERS_PATH` to point to the path of where the
*flu_va_drivers_vdpau_drv_video.so* file is located.

## Google Chrome (Chromium)

In order to get Google Chrome using this project, you have to run Google Chrome
as following:
```sh
LIBVA_DRIVER_NAME=flu_va_drivers_vdpau \
LIBVA_DRIVERS_PATH=$(pkg-config --variable=prefix libva) \
google-chrome --enable-features=VaapiVideoDecoder --ignore-gpu-blocklist --use-gl=desktop
```

Note, in latest versions of Google Chrome, also add:
`--disable-features=UseChromeOSDirectVideoDecoder`

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

# Testing

## VDPAU

The project is on develop and there is no way to do a full decoding test. So by
now the way we're testing consist on try to decode using `ffmpeg` with this
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
