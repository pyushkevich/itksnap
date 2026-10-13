// Test: undo and redo of segmentation edits (LabelImageWrapper / UndoDataManager).
//
// Edits are made the way the drawing tools make them, with a
// SegmentationUpdateIterator whose delta is committed as an undo point. After
// each step the whole segmentation is compared with a snapshot.
//
//   1. Two edits, undone and redone in order.
//   2. A new edit after an undo discards the redo history.
//   3. An edit that changes nothing creates no undo point, and does not
//      discard the redo history.
//   4. Several deltas committed as one undo point are undone together.
//   5. Label differences that wrap around the 16-bit label range.
//   6. Each segmentation layer has its own history.
//   7. The history size limit drops the oldest undo points.
//   8. Each time point of a 4D segmentation has its own history.
//
// Usage: TestUndoRedo <testdata_dir> <temp_dir>
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageIODelegates.h"
#include "LabelImageWrapper.h"
#include "SegmentationUpdateIterator.h"
#include "UIReporterDelegates.h"
#include "IRISException.h"
#include <itksys/SystemTools.hxx>
#include <iostream>
#include <string>
#include <vector>

class SimpleSystemInfoDelegate : public SystemInfoDelegate
{
public:
  SimpleSystemInfoDelegate(const char *argv0, std::string tmp)
    : m_Exe(argv0), m_Temp(tmp) {}
  std::string GetApplicationDirectory() override
    { return itksys::SystemTools::GetFilenamePath(m_Exe); }
  std::string GetApplicationFile() override { return m_Exe; }
  std::string GetApplicationPermanentDataLocation() override { return ".itksnap_test"; }
  std::string GetUserDocumentsLocation() override { return ".itksnap_test"; }
  std::string GetTempDirectory() override { return m_Temp; }
  std::string EncodeServerURL(const std::string &url) override { return url; }
  void LoadResourceAsImage2D(std::string, GrayscaleImage *) override {}
  void LoadResourceAsRegistry(std::string, Registry &) override {}
  void WriteRGBAImage2D(std::string, RGBAImageType *) override {}
private:
  std::string m_Exe, m_Temp;
};

class SimpleColorMapSource : public AbstractColorMapPresetNameSource
{
public:
  std::string GetPresetName(ColorMap::SystemPreset p, bool) override
    { return "Preset " + std::to_string(static_cast<int>(p)); }
};

using RegionType = itk::ImageRegion<3>;
using Snapshot = std::vector<LabelType>;

static int n_failures = 0;

static void Check(bool cond, const std::string &what)
{
  std::cout << (cond ? "  ok:   " : "  FAIL: ") << what << std::endl;
  if(!cond)
    n_failures++;
}

static RegionType Box(long x, long y, long z, unsigned long size)
{
  RegionType r;
  r.SetIndex(0, x); r.SetIndex(1, y); r.SetIndex(2, z);
  r.SetSize(0, size); r.SetSize(1, size); r.SetSize(2, size);
  return r;
}

// All voxels of the current time point of a segmentation
static Snapshot Snap(LabelImageWrapper *seg)
{
  Snapshot s;
  LabelImageWrapper::ImageType *img = seg->GetModifiableImage();
  itk::ImageRegionConstIterator<LabelImageWrapper::ImageType> it(img, img->GetBufferedRegion());
  for(; !it.IsAtEnd(); ++it)
    s.push_back(it.Get());
  return s;
}

// Paint a label over a region, as the drawing tools do. With an undo name the
// change is committed as an undo point; without one the delta is returned so
// that the caller can stage it. Returns whether any voxel changed.
static bool Paint(LabelImageWrapper *seg, const RegionType &region, LabelType label,
                  const char *undo_name, SegmentationUpdateIterator::UndoDelta **delta = nullptr)
{
  DrawOverFilter all;
  all.CoverageMode = PAINT_OVER_ALL;
  all.DrawOverLabel = 0;
  SegmentationUpdateIterator it(seg, region, label, all);
  for(; !it.IsAtEnd(); ++it)
    it.PaintAsForeground();
  bool changed = it.Finalize(undo_name);
  if(delta)
    *delta = changed ? it.RelinquishDelta() : nullptr;
  return changed;
}

// Paint label on every other voxel of the whole image (checkerboard), which
// gives a delta with about one RLE per voxel
static void PaintCheckerboard(LabelImageWrapper *seg, LabelType label, const char *undo_name)
{
  DrawOverFilter all;
  all.CoverageMode = PAINT_OVER_ALL;
  all.DrawOverLabel = 0;
  RegionType region = seg->GetModifiableImage()->GetBufferedRegion();
  SegmentationUpdateIterator it(seg, region, label, all);
  for(; !it.IsAtEnd(); ++it)
    {
    auto idx = it.GetIndex();
    if((idx[0] + idx[1] + idx[2]) % 2 == 0)
      it.PaintAsForeground();
    }
  it.Finalize(undo_name);
}

int main(int argc, char *argv[])
{
  if(argc < 3)
    {
    std::cerr << "Usage: " << argv[0] << " <testdata_dir> <temp_dir>" << std::endl;
    return 1;
    }

  std::string data_dir = argv[1], temp_dir = argv[2];
  itksys::SystemTools::MakeDirectory(temp_dir);
  SimpleSystemInfoDelegate sidel(argv[0], temp_dir);
  SystemInterface::SetSystemInfoDelegate(&sidel);
  SimpleColorMapSource cmSource;
  ColorMap::SetColorMapPresetNameSource(&cmSource);

  try
    {
    // MRIcrop: 78 x 110 x 64 voxels, with a blank segmentation
    auto app = IRISApplication::New();
    IRISWarningList wl;
    app->OpenImage((data_dir + "/MRIcrop-orig.gipl.gz").c_str(), MAIN_ROLE, wl);
    LabelImageWrapper *seg = app->GetSelectedSegmentationLayer();
    Check(seg != nullptr, "main image has a segmentation");

    // ── 1. Undo and redo in order ─────────────────────────────────────────
    std::cout << "Undo and redo:" << std::endl;
    Snapshot s0 = Snap(seg);
    Check(!app->IsUndoPossible() && !app->IsRedoPossible(), "no history after loading");
    Paint(seg, Box(10, 10, 10, 20), 1, "A");
    Snapshot sA = Snap(seg);
    Paint(seg, Box(20, 20, 20, 20), 2, "B");      // overlaps A
    Snapshot sAB = Snap(seg);
    app->Undo();
    Check(Snap(seg) == sA, "undo B restores A, including the overlap");
    app->Undo();
    Check(Snap(seg) == s0, "undo A restores the empty segmentation");
    Check(!app->IsUndoPossible(), "nothing left to undo");
    app->Redo();
    Check(Snap(seg) == sA, "redo A");
    app->Redo();
    Check(Snap(seg) == sAB, "redo B");
    Check(!app->IsRedoPossible(), "nothing left to redo");

    // ── 2. A new edit after an undo discards the redo history ─────────────
    std::cout << "New edit after undo:" << std::endl;
    app->Undo();
    Check(app->IsRedoPossible(), "redo possible after undo");
    Paint(seg, Box(0, 0, 0, 5), 3, "C");
    Snapshot sAC = Snap(seg);
    Check(!app->IsRedoPossible(), "new edit discards the redo history");
    app->Undo();
    Check(Snap(seg) == sA, "undo C restores A");
    app->Redo();
    Check(Snap(seg) == sAC, "redo C");

    // ── 3. An edit that changes nothing ───────────────────────────────────
    std::cout << "Edit that changes nothing:" << std::endl;
    Check(!Paint(seg, Box(0, 0, 0, 5), 3, "C again"), "painting the same label changes nothing");
    Check(!seg->StoreUndoPoint("empty"), "storing an undo point with no changes creates none");
    app->Undo();
    Check(Snap(seg) == sA, "undo still reverts the last real edit (C)");
    Check(!seg->StoreUndoPoint("empty after undo"), "empty undo point after an undo creates none");
    Check(app->IsRedoPossible(), "empty undo point after an undo keeps the redo history");
    if(app->IsRedoPossible())
      app->Redo();
    else
      Paint(seg, Box(0, 0, 0, 5), 3, "C");
    Check(Snap(seg) == sAC, "back to A + C");

    // ── 4. Several deltas in one undo point ───────────────────────────────
    std::cout << "Several deltas in one undo point:" << std::endl;
    SegmentationUpdateIterator::UndoDelta *d1 = nullptr, *d2 = nullptr;
    Paint(seg, Box(40, 40, 40, 10), 4, nullptr, &d1);
    Paint(seg, Box(45, 45, 45, 10), 5, nullptr, &d2);
    Check(d1 && d2, "both strokes changed voxels");
    seg->StoreIntermediateUndoDelta(d1);
    seg->StoreIntermediateUndoDelta(d2);
    Check(seg->StoreUndoPoint("two strokes"), "undo point with two deltas");
    Snapshot sTwo = Snap(seg);
    app->Undo();
    Check(Snap(seg) == sAC, "one undo reverts both strokes");
    app->Redo();
    Check(Snap(seg) == sTwo, "one redo reapplies both strokes");

    // ── 5. Label differences that wrap around ─────────────────────────────
    std::cout << "Labels at the ends of the range:" << std::endl;
    Snapshot sBefore = Snap(seg);
    Paint(seg, Box(10, 10, 10, 20), 65535, "max label over label 1 and 2");
    Snapshot sMax = Snap(seg);
    Paint(seg, Box(15, 15, 15, 20), 0, "clear over max label");
    app->Undo();
    Check(Snap(seg) == sMax, "undo clear over label 65535");
    app->Undo();
    Check(Snap(seg) == sBefore, "undo label 65535 over labels 1 and 2");
    app->Redo();
    app->Redo();
    app->Undo();
    app->Undo();
    Check(Snap(seg) == sBefore, "redo and undo again");

    // ── 6. Each segmentation has its own history ──────────────────────────
    std::cout << "Two segmentations:" << std::endl;
    Snapshot seg1State = Snap(seg);
    app->AddBlankSegmentation();
    LabelImageWrapper *seg2 = app->GetSelectedSegmentationLayer();
    Check(seg2 && seg2 != seg, "new segmentation is selected");
    Check(!app->IsUndoPossible(), "new segmentation has no history");
    Snapshot seg2Empty = Snap(seg2);
    Paint(seg2, Box(30, 30, 30, 10), 7, "seg2 edit");
    app->Undo();
    Check(Snap(seg2) == seg2Empty, "undo in the second segmentation");
    Check(Snap(seg) == seg1State, "first segmentation unchanged");
    app->GetCurrentImageData()->SetActiveSegmentationLayer(seg->GetUniqueId());
    Check(app->GetSelectedSegmentationLayer() == seg, "first segmentation selected again");
    Check(app->IsUndoPossible(), "first segmentation keeps its history");
    // Section 5 ended by undoing its own edits, so the next undo reverts the
    // two strokes of section 4
    app->Undo();
    Check(Snap(seg) == sAC, "undo in the first segmentation continues where it left off");

    // ── 7. History size limit ─────────────────────────────────────────────
    // Each checkerboard edit has about one RLE per voxel (549k), more than the
    // 200000 allowed, so only the minimum number of undo points (4) is kept,
    // plus the newest
    std::cout << "History size limit:" << std::endl;
    seg->ClearUndoPoints();
    std::vector<Snapshot> states;
    states.push_back(Snap(seg));
    for(LabelType k = 1; k <= 7; k++)
      {
      PaintCheckerboard(seg, k, "checkerboard");
      states.push_back(Snap(seg));
      }
    int n_undo = 0;
    while(app->IsUndoPossible() && n_undo < 10)
      {
      app->Undo();
      n_undo++;
      }
    Check(n_undo == 5, "5 undo points kept (got " + std::to_string(n_undo) + ")");
    Check(Snap(seg) == states[7 - n_undo], "oldest kept state restored correctly");
    for(int i = 0; i < n_undo; i++)
      app->Redo();
    Check(Snap(seg) == states[7], "redo back to the newest state");
    }
  catch(std::exception &exc)
    {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return 1;
    }

  try
    {
    // ── 8. Time points of a 4D segmentation (38 x 35 x 38 x 11) ───────────
    std::cout << "4D time points:" << std::endl;
    auto app4d = IRISApplication::New();
    IRISWarningList wl;
    app4d->OpenImage((data_dir + "/img4d_11f.nii.gz").c_str(), MAIN_ROLE, wl);
    LabelImageWrapper *seg = app4d->GetSelectedSegmentationLayer();
    Check(app4d->GetNumberOfTimePoints() == 11, "11 time points");

    app4d->SetCursorTimePoint(0);
    Snapshot t0Empty = Snap(seg);
    Paint(seg, Box(5, 5, 5, 10), 1, "t0 edit");
    Snapshot t0Edited = Snap(seg);

    app4d->SetCursorTimePoint(3);
    Check(!app4d->IsUndoPossible(), "time point 3 has no history");
    Snapshot t3Empty = Snap(seg);
    Paint(seg, Box(10, 10, 10, 10), 2, "t3 edit");
    app4d->Undo();
    Check(Snap(seg) == t3Empty, "undo at time point 3");

    app4d->SetCursorTimePoint(0);
    Check(Snap(seg) == t0Edited, "time point 0 unchanged by undo at time point 3");
    Check(app4d->IsUndoPossible(), "time point 0 keeps its history");
    app4d->Undo();
    Check(Snap(seg) == t0Empty, "undo at time point 0");
    }
  catch(std::exception &exc)
    {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return 1;
    }

  std::cout << (n_failures ? "FAILED: " : "PASSED: ") << n_failures << " failure(s)" << std::endl;
  return n_failures ? 1 : 0;
}
