#ifndef IMAGEIOWIZARDMODEL_H
#define IMAGEIOWIZARDMODEL_H

#include <string>

#include "GuidedNativeImageIO.h"
#include "Registry.h"

class GlobalUIModel;

/**
 This class provides a model for the ImageIO wizards. This allows the wizard
 to be distanced from the program logic. The wizard is just a collection of
 buttons and callbacks, but very little state

 This class is subclassed by specific dialogs, to allow customization. For
 example, save/load dialog behavior is handled this way.
 */
class ImageIOWizardModel : public itk::Object
{
public:
  typedef GuidedNativeImageIO::FileFormat FileFormat;
  enum Mode { LOAD, SAVE };

  enum SummaryItem {
    SI_FILENAME, SI_DIMS, SI_SPACING, SI_ORIGIN, SI_ORIENT,
    SI_ENDIAN, SI_DATATYPE, SI_FILESIZE
  };

  ImageIOWizardModel(GlobalUIModel *parent, Mode mode, const char *name);

  irisGetMacro(GuidedIO, GuidedNativeImageIO *)

  virtual ~ImageIOWizardModel();

  // Does the model support loading
  bool IsLoadMode() const { return m_Mode == LOAD; }

  // Does the model support loading
  bool IsSaveMode() const { return m_Mode == SAVE; }

  // Whether we can save or load a file format
  virtual bool CanHandleFileFormat(FileFormat fmt);

  // This another method that checks if a loaded image is valid
  virtual bool CheckImageValidity();

  // Default extension for saving files
  virtual std::string GetDefaultExtensionForSave() const
    { return std::string("nii.gz"); }

  /**
    Create a filter string for file IO dialogs. The lineEntry is in the
    printf format, with first %s being the title of the format, and the
    second being the list of extensions. extEntry is similar, used to print
    each extension. The examples for Qt are "%s (%s)" for lineEntry and
    "*.%s" for extEntry. For FLTK it would be "%s *.{%s}" for lineEntry
    and "%s" for extEntry. The separators are used to separate extensions
    per entry and rows in the filter string.
   */
  std::string GetFilter(const char *lineEntry,
                        const char *extEntry,
                        const char *extSeparator,
                        const char *rowSeparator);

  /**
    Guess the format for the file. In load mode, if the file does not exist,
    this will return FORMAT_COUNT, i.e., failure to determine format. If it
    exists, the format will be determined using registry information (if open
    before), magic number, and extension, in turn. If in save mode, format
    is detected using registry and extension only. The last parameter is only
    considered in Load mode.
    */
  FileFormat GuessFileFormat(const std::string &fname, bool &fileExists);

  /**
    Get the directory where to browse, given a currently entered file
    */
  std::string GetBrowseDirectory(std::string &file);

  /**
    Get the history of filenames
    */
  typedef std::vector<std::string> HistoryType;
  HistoryType GetHistory() const;

  /**
    * Reset the state of the model
    */
  void Reset();

  /**
    Set the format selected by the user
    */
  void SetSelectedFormat(FileFormat fmt);

  /**
    Load the image from filename
    */
  void LoadImage(std::string filename);

  /**
    Get summary items to display for the user
    */
  std::string GetSummaryItem(SummaryItem item);


protected:

  // State of the model
  Mode m_Mode;

  // The history list associated with the model
  std::string m_HistoryName;

  // Parent model
  GlobalUIModel *m_Parent;
  GuidedNativeImageIO *m_GuidedIO;

  // Registry containing auxiliary info
  Registry m_Registry;

  // Filename picked by the user
  std::string m_Filename;
};

#endif // IMAGEIOWIZARDMODEL_H
