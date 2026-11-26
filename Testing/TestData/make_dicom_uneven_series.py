"""
Write the synthetic MR series in dicom_uneven_series/ used by the
DicomSeriesDetection test (#202): 8 slice positions with one image each, plus
a second image at position 3, as in a perfusion series with an extra image.
ITK-SNAP used to fail on such a series with "vector too long".

  Files m_0001 - m_0008: position 2 * (n - 1) mm, intensity 1000 + n - 1
  File  m_0009:          position 6 mm (like m_0004), intensity 1500

Usage: python make_dicom_uneven_series.py dicom_uneven_series
"""
import os, sys, numpy as np
from pydicom.dataset import Dataset, FileMetaDataset
from pydicom.uid import ExplicitVRLittleEndian, generate_uid, MRImageStorage

out = sys.argv[1]
os.makedirs(out, exist_ok=True)

# Fixed UIDs, so that the files only change when this script does
def uid(*key):
    return generate_uid(entropy_srcs=["itksnap-dicom-uneven-series"] + [str(k) for k in key])

study, series = uid("study"), uid("series")

def write_image(inst, z, value, size=16):
    meta = FileMetaDataset()
    meta.MediaStorageSOPClassUID = MRImageStorage
    meta.MediaStorageSOPInstanceUID = uid(inst)
    meta.TransferSyntaxUID = ExplicitVRLittleEndian
    ds = Dataset(); ds.file_meta = meta
    ds.SOPClassUID = MRImageStorage
    ds.SOPInstanceUID = meta.MediaStorageSOPInstanceUID
    ds.Modality = "MR"; ds.Manufacturer = "SIEMENS"
    ds.PatientName = "Synthetic^Test"; ds.PatientID = "0"
    ds.StudyInstanceUID = study; ds.SeriesInstanceUID = series
    ds.SeriesNumber = 5; ds.SeriesDescription = "Perfusion uneven"
    ds.InstanceNumber = inst
    ds.ImagePositionPatient = [0.0, 0.0, float(z) * 2.0]
    ds.ImageOrientationPatient = [1, 0, 0, 0, 1, 0]
    ds.SliceLocation = float(z) * 2.0
    ds.SliceThickness = "2.0"
    ds.PixelSpacing = [1.0, 1.0]
    ds.Rows = size; ds.Columns = size
    ds.SamplesPerPixel = 1; ds.PhotometricInterpretation = "MONOCHROME2"
    ds.BitsAllocated = 16; ds.BitsStored = 16; ds.HighBit = 15
    ds.PixelRepresentation = 1
    ds.RescaleIntercept = 0; ds.RescaleSlope = 1
    ds.PixelData = np.full((size, size), value, dtype=np.int16).tobytes()
    ds.save_as(os.path.join(out, f"m_{inst:04d}.dcm"), enforce_file_format=True)

for z in range(8):
    write_image(z + 1, z, 1000 + z)
write_image(9, 3, 1500)
