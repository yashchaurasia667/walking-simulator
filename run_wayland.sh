#!/bin/bash
__NV_PRIME_RENDER_OFFLOAD=1 \
__EGL_VENDOR_LIBRARY_FILENAMES=/usr/share/glvnd/egl_vendor.d/10_nvidia.json \
GBM_BACKEND=nvidia-drm \
./build/terraingen
