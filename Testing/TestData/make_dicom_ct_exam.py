"""
Write the synthetic CT exam in dicom_ct_exam/ used by the DicomSeriesDetection
tests (#218): two 3D series and one 4D CTA series (several time frames at each
slice position), all from a GE scanner, in one directory.

In series 3, the first file has SliceThickness "2.000000" and the others "2.0",
as some scanners write it (#128). It must still be read as a single series.

  Series 2 "Axial 3D A":  8 slices,               intensity 1000 + slice
  Series 3 "Axial 3D B":  6 slices,               intensity 2000 + slice
  Series 4 "Dynamic CTA": 5 slices x 3 frames,    intensity 100 * frame + slice

Usage: python make_dicom_ct_exam.py dicom_ct_exam
"""
import os, sys, numpy as np
from pydicom.dataset import Dataset, FileMetaDataset
from pydicom.uid import ExplicitVRLittleEndian, generate_uid, CTImageStorage

out = sys.argv[1]
os.makedirs(out, exist_ok=True)

# Fixed UIDs, so that the files only change when this script does
def uid(*key):
    return generate_uid(entropy_srcs=["itksnap-dicom-ct-exam"] + [str(k) for k in key])

study = uid("study")

def write_series(number, desc, n_pos, n_frames, base, size=16):
    inst = 0
    for f in range(n_frames):
        for z in range(n_pos):
            inst += 1
            meta = FileMetaDataset()
            meta.MediaStorageSOPClassUID = CTImageStorage
            meta.MediaStorageSOPInstanceUID = uid(number, inst)
            meta.TransferSyntaxUID = ExplicitVRLittleEndian
            ds = Dataset(); ds.file_meta = meta
            ds.SOPClassUID = CTImageStorage
            ds.SOPInstanceUID = meta.MediaStorageSOPInstanceUID
            ds.Modality = "CT"; ds.Manufacturer = "GE MEDICAL SYSTEMS"
            ds.PatientName = "Synthetic^Test"; ds.PatientID = "0"
            ds.StudyInstanceUID = study; ds.SeriesInstanceUID = uid(number)
            ds.SeriesNumber = number; ds.SeriesDescription = desc
            ds.InstanceNumber = inst
            ds.ImagePositionPatient = [0.0, 0.0, float(z) * 2.0]
            ds.ImageOrientationPatient = [1, 0, 0, 0, 1, 0]
            ds.SliceLocation = float(z) * 2.0
            ds.SliceThickness = "2.000000" if (number == 3 and inst == 1) else "2.0"
            ds.PixelSpacing = [1.0, 1.0]
            ds.Rows = size; ds.Columns = size
            ds.SamplesPerPixel = 1; ds.PhotometricInterpretation = "MONOCHROME2"
            ds.BitsAllocated = 16; ds.BitsStored = 16; ds.HighBit = 15
            ds.PixelRepresentation = 1
            ds.RescaleIntercept = 0; ds.RescaleSlope = 1
            ds.PixelData = np.full((size, size), base + 100 * f + z, dtype=np.int16).tobytes()
            ds.save_as(os.path.join(out, f"s{number}_{inst:04d}.dcm"), enforce_file_format=True)

write_series(2, "Axial 3D A", 8, 1, 1000)
write_series(3, "Axial 3D B", 6, 1, 2000)
write_series(4, "Dynamic CTA", 5, 3, 0)
