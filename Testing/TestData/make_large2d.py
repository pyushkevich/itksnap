#!/usr/bin/env python3
"""Generate the synthetic large 2D test images (large2d_*).

Images wider than 1024 voxels are always sliced with the non-orthogonal slicer
(ImageWrapper::CanOrthogonalSlicingBeUsed). These images exercise that path
for 2D data. The main image is a pattern of 128x128 blocks with known values;
each overlay holds 255 minus the main image, so main + overlay = 255 wherever
both are sampled correctly. The block pattern keeps the files a few KB.
The Logic test TestLarge2DSlicing makes more overlay geometries in memory.

Usage: make_large2d.py <output_dir>   (needs numpy and SimpleITK)
"""
import os
import sys
import numpy as np
import SimpleITK as sitk

# Wider than 1024, so slicing is non-orthogonal; kept small for file size
NX, NY = 1280, 640
SPACING = (0.5, 0.5, 2.0)
ORIGIN = (10.0, -20.0, 5.0)


def main_values(nx=NX, ny=NY, scale=1):
    """Block pattern: 20 + (7*bx + 13*by) % 200 for 128x128 blocks (in main voxels)"""
    y, x = np.mgrid[0:ny, 0:nx] * scale
    return (20 + (7 * (x // 128) + 13 * (y // 128)) % 200).astype(np.uint8)


def write(arr2d, fn, spacing=SPACING, origin=ORIGIN, direction=(1, 0, 0, 0, 1, 0, 0, 0, 1)):
    img = sitk.GetImageFromArray(arr2d[np.newaxis, :, :])
    img.SetSpacing(spacing)
    img.SetOrigin(origin)
    img.SetDirection(direction)
    sitk.WriteImage(img, fn, useCompression=True)


def main(outdir):
    m = main_values()
    write(m, os.path.join(outdir, "large2d_main.nii.gz"))

    # Overlays: 255 - main, with a geometry that differs from the main image
    ov = (255 - m).astype(np.uint8)
    # Plane shifted by 0.3 voxel along z: a small disagreement, as between a 2D
    # image and an overlay exported by another program
    write(ov, os.path.join(outdir, "large2d_ov_zshift.nii.gz"),
          origin=(ORIGIN[0], ORIGIN[1], ORIGIN[2] + 0.3 * SPACING[2]))
    # Half the resolution in plane, covering the same area
    ovh = (255 - main_values(NX // 2, NY // 2, 2)).astype(np.uint8)
    write(ovh, os.path.join(outdir, "large2d_ov_halfres.nii.gz"),
          spacing=(2 * SPACING[0], 2 * SPACING[1], SPACING[2]),
          origin=(ORIGIN[0] + 0.5 * SPACING[0], ORIGIN[1] + 0.5 * SPACING[1], ORIGIN[2]))

    # Segmentation on the main grid: label 1 and label 2 rectangles
    seg = np.zeros((NY, NX), np.uint16)
    seg[128:256, 256:512] = 1
    seg[384:512, 768:1024] = 2
    write(seg, os.path.join(outdir, "large2d_seg.nii.gz"))

    # Segmentation covering only part of the main image, all label 1
    crop = np.ones((320, 512), np.uint16)
    write(crop, os.path.join(outdir, "large2d_seg_crop.nii.gz"),
          origin=(ORIGIN[0] + 384 * SPACING[0], ORIGIN[1] + 160 * SPACING[1], ORIGIN[2]))

    # Three-component (RGB) image (issue #200: large 2D multicomponent images did
    # not render). ITK-SNAP does not read PNG, so this is a NIfTI vector image.
    rgb = np.stack([m, 255 - m, (m // 2 + 64).astype(np.uint8)], axis=-1)
    img = sitk.GetImageFromArray(rgb[np.newaxis, :, :, :], isVector=True)
    img.SetSpacing(SPACING)
    img.SetOrigin(ORIGIN)
    sitk.WriteImage(img, os.path.join(outdir, "large2d_rgb.nii.gz"), useCompression=True)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else ".")
