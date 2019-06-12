ITK-SNAP Release Notes - Version 3.8
====================================

## Version 3.8.0
----

This version introduces an exciting new feature: the **Distributed Segmentation Service (DSS)**. This service allows developers to make various image segmentation algorithms available to you directly in ITK-SNAP. Several algorithms have already been made available: most of them focused on hippocampus segmentation, and we expect more to appear soon. DSS is a web-based system, so data is sent to a server and processed by algorithm providers using their own computer hardware.

### New features

- New GUI for the DSS, under *Tools->Distributed Segmentation Service*
    - User and developer documentation for DSS at https://alfabis-server.readthedocs.io/en/latest/
- New support for multiple segmentation layers (switch between them with **{**,**}** keys)
- Command-line functionality for interacting with the DSS service as a user or developer, part of **itksnap-wt** workspace editor tool
- New functionality for plotting time-course data in the *Image Information* dialog
- Implemented support for reading DICOM data with multiple images per z-position
- Registration interface has improved image resizing support, support for flipping
- Command-line tools **c3d** and **greedy** now can be installed using *Help->Install Command-Line Tools*
- Support for reading "Generic ITK Images" in the "File->Open" dialog, makes it possible to open any image that ITK can read

### Bug Fixes

- Fixed crashes that occurred occasionally when loading multiple images with different orientations
- Enabled loading of any image file supported by ITK via "Generic ITK" option in IO dialogs
- Fixed crash when canceling random forest segmentation
- Fixed crash when loading images of type **char**
- Fixed issue with segmentation labels not resetting when loading a new image
- Keyboard shortcuts Ctrl-1 to Ctrl-5 for loading recent images restored

## Version 3.6.2
----

The main change in this version is the addition of the workspace building tool itksnap_ws. This tool will form the basis of the distributed segmentation system in ITK-SNAP.

- Added tool itksnap_ws (in Utilities/Workspace)


## Version 3.6.0
----

The main focus of this release is on making it easier to work with multiple images that have different size and resolution. This is particularly useful for viewing and segmenting MRI studies, where a single session contains many scans with different parameters. Important new features include the ability to load multiple images of different dimensions, voxel size, and orientation into a single ITK-SNAP window; automatic and manual **registration**; and enhanced support for DICOM format images. With these new features, researchers who work with MRI DICOM datasets will find it much easier to incorporate ITK-SNAP into their workflow. Another important new feature in this release is the ability to **interpolate** segmentation between slices. This makes it possible to create manual segmentations much more quickly than before.

### New features

- Multiple images with different dimensions, voxel size, and orientation can be visualized in the same ITK-SNAP window. When additional images are loaded, they are represented in memory in their native resolution, and resampled on the fly to match the screen resolution.

    - This means that you can use information from two MRI modalities to guide manual segmentation. You can load a T1-weighted image with 1.0mm isotropic resolution and a T2-weighted image with 0.4mm x 0.4mm x 2.0mm resolution, and use the full information from both of these images for your segmentation. 

    - The manual segmentation is still performed in the voxel space of the main image. 

- A new registration tool is available under *Tools->Registration*. The tool provides both interactive manual registration and automatic affine and rigid registration.

    - Manual registration includes rotation/translation/scaling widgets and a mouse-based interactive registration mode, where moving the mouse over the 'moving' image performs rotation and translation. Rotation is performed by turning a 'wheel' widget, and very small rotations are possible. The center of rotation can be set by moving the cursor.

    - Automatic registration is quite fast. It allows rigid and affine registration. It supports mutual information (inter-modality) and patch cross-correlation (intra-modality) metrics. Optionally, a mask can be provided, over which the metric is computed. It is easy to generate masks using the segmentation interpolation tool (see below). Masks are useful when the extent of the images is different, e.g., one includes neck and another does not. 
    
    - Registration results can be saved as matrix files compatible with ANTS, Convert3D and Greedy tools. Using Convert3D, they can be converted to FLIRT-compatible transform files. Registration results are also automatically saved in workspace files. Images can also be resliced into the space of the main image.
    
- DICOM functionality is greatly improved. 

    - After you load a 3D volume from a DICOM image directory, you can load other 3D volumes in the same directory quickly using the new *File->Add Another DICOM Series* submenu.
    
    - Listing of DICOM directories (in the Open Image wizard) is much faster than before. This really makes a difference when opening images on DVDs and USB sticks.
    
    - Dragging and dropping a DICOM file onto the ITK-SNAP window on Windows and Mac now shows the list of 3D volumes in the directory, allowing you to choose a volume to open.
    
- A new tool for interpolating segmentations between slices under *Tools->Interpolate Labels*. 

    - For images with thin slices and gradually changing anatomy, you can segment every fifth slice and fill in the missing slices using this new tool. Details of the algorithm are provided in an [Insight Journal article](http://insight-journal.org/browse/publication/977). Interpolation can be performed for a specific label or for all labels.
    
- Label visibility can be changed 'en masse' in the Label Editor, i.e., you can make all labels visible or hidden. This helps find a label in segmentation with many labels that occlude each other. 

- A new ''grid'' mode is provided for visualizing displacement fields. Displacement fields in formats used by [ANTS](http://stnava.github.io/ANTs/) and [Greedy](https://sites.google.com/view/greedyreg/about) are currently supported.

- The user interface automatically scales to reasonable size on very high DPI displays, such as the Miscrosoft Surface (tm). Environment flag QT_SCALE_FACTOR can be used to override.

### Programmatic Enhancements
- ITK-SNAP now includes Convert3D and Greedy registration tools as submodules. This is already used to support registration functionality, but in the future we will be adding more Convert3D-based functionality, such as filtering, etc.

- The ImageWrapper class includes a dual slicing module that selects between orthogonal and non-orthogonal slicing based on image orientation. This is significantly cleaned up relative to earlier versions.

- ITK-SNAP now compatible with Qt 5.6, which is the standard for the next 3 years

### Bug Fixes 

- Fixed issue with *Workspace->Save As...* where you would be asked to save the current workspace (thx Laura W).
- Fixed issue where DICOM data was not loading correctly when workspace was moved in the filesystem (thx Vincent F).
- Fixed issue with loading of NRRD images on some systems due to non-US locale (thx Johan B)
- Fixed behavior of *Ctrl-W*, so that when there are open dialog windows this keystroke closes them rather than the current image layer (thx Karl B)
- Fixed issue with multi-component images, where every 100th component showed up as 'Magnitude' (this Phil. C)
- Fixed OpenGL rendering issues on Windows 10 on some cards
- Fixed computation of zoom factor on Retina displays
- Fixed behavior of File Save browse dialogs on MacOS when saving .nii.gz files
- Fixed issues with unnecessary prompting to save segmentations
- Improved OpenGL compatibility, particularly in VTK-managed windows
- Fixed issues with images >4GB not opening on Windows. This requires building ITK with ITK_USE_64BITS_IDS=TRUE
- Fixed problems with histories not showing up in file dialogs
- Fixed bug where scalpel tool did not work for anisotropic images
- Several other fixes for specific crashes.

### Known Issues

 -  Some advanced features (label editor advanced tools, window shrinking) have
    not yet been ported to Qt. They will be ported in future versions, and
    expanded in the process. There is a plan to integrate SNAP and C3D which
    will provide much richer image processing support for images and segmentations.

 -  The cutplane in the scalpel tool is too big when the camera is zoomed in and
    it needs a flip button. Also with dark labels you can't see the handle.

## Version 3.4.0
----
This release was largely focused on improving the user experience through extensive
changes to the GUI. We made access to the new features introduced in versions 3.0 and
3.2 more intuitive. The loading and display of multiple images has been extensively
reworked and improved. In addition, we made the classification mode in the semi-
automatic segmentation workflow more powerful by providing additional image and
geometric features that the classifier can use to learn the patterns of difference
between the classes labeled by the use. We have been alsobusy working on new
functionality such as image registration within SNAP, and a smaller memory footprint,
but these will become available in the 3.6 release. The main focus of 3.4 is
on usability.

### New Features

 -  The controls for selecting segmentation labels have been brought back to the
    main SNAP panel, where they were in versions 1 and 2. This will make the
    interface more familiar to long-time users.

 -  The interface for loading and viewing additional image layers has been
    extensively redesigned. When more than one anatomical image is loaded into
    ITK-SNAP, the default behavior is to display one image at full size and the
    rest of the images as thumbnails. Clicking the thumbnails makes them full
    size. This new thumbnail mode is far more intuitive for working with
    multiple images than the method of semi-transparent overlays employed in
    previous versions.

 -  The IO dialog for opening additional images promts whether the user would
    like the image to be shown side by side with an existing image or as a
    semi-transparent overlay.

 -  The thumbnails have a context menu button that provides fast access to
    common image commands, such as adjusting contrast and color map.

 -  The tiled display mode introduced in version 3.0 is also available as an
    alternative to the thumbnail display.

 -  The new thumbnail mode makes semi-automatic segmentation easier to work
    with because one can switch easily between anatomical images and the
    speed (blue and white) image during pre-segmentation, bubble placement
    and contour evolution.

 -  A new image annotation tool has been added to the main tool palette. This
    tool can be used to measure distances on image slices, draw lines, measure
    angles between lines, and place text annotations in specific points in the
    3D image. Annotations can be exported and imported, allowing teams to
    comment on medical images and segmentations. Annotations are automatically
    saved in workspace files.

 -  Command tooltips have been redesigned and now include information on the
    shortcut key to activate each command. In addion, tooltips for the tools in
    the main palette describe the actions of each mouse button. Many new shortcuts
    have been added.

 -  Cleaned up the behavior of mouse buttons and mouse scrolling to be more
    consistent between modes. Added Shift-scroll action, which scrolls through
    timepoints in a 4D image and components in a 3D multi-component image.

 -  Added the ability to generate "contextual" features in the classification
    mode in the semi-automatic segmentation workflow. Two types of contextual
    features are provided: neighboring intensity features, and coordinate
    features. The former allow the classifier to learn patterns of textural
    differences between regions in the image. When used, the classifier can
    separate regions that have identical average intensity but different texture.
    The coordinate features allow the classifier to learn geometrical information,
    such as the location/extent of the structure of interest relative to the
    background structures. This can be used to segment a structure that has
    no intensity contrast with adjacent structures, simply by giving the classifer
    hints as to the location of the boundary on a few slices. These new features
    are available by pressing "More..." when in classification mode.

 -  Added some extra controls when working in classificaiton mode, such as
    setting the forest size and tree depth for the random forest classifier
    and a slider for "classifier bias", which allows you to bias the classfier
    output more toward the foreground class or background classes.

 -  Added the ability to select more than one class as the foreground class in
    the random forest classification mode. This is powerful when the foreground
    object has heterogeneous intensity.

 -  Training examples drawn in random forest classificaiton mode are now retained
    and can be reused to label multiple structures.

### Programmatic Improvements

 -  Refactored the software to allow multiple images that occupy different
    anatomical space to be loaded in the same ITK-SNAP session. This
    functionality has not yet been enabled, and will be rolled out it 3.4 as
    part of the new image registration functionality.

 -  Updated to Qt 5.4

 -  Fixed font size and other rendering issues on Retina displays. Added
    high-resolution icons for Retina.

 -  Made it possible to build ITK-SNAP against older Qt version 4.8. In some
    Linux environments, applications based on Qt5 do not work well over remote
    connection (ssh -X, VNC, x2go, NX) because the way Qt and X11 interface was
    changed drastically in Qt5. The fallback to Qt4 makes it possible to run the
    new SNAP version in cluster environments.

 -  Fixed problems with loading images > 4gb

 -  Restored ability to build on 64 bit Windows

## Version 3.2.0
----
The main new feature introduced in this release is supervised classification.
The release also is built against Qt5 (3.0 used Qt4.8), which resulted in a lot
of changes to compilation and packaging. Quite a few bugs have been fixed and
the release should be more stable than 3.0.

### New Features

 -  Added a supervised classification presegmentation mode. This mode allows
    the user to compute the speed image by marking examples of two or more
    tissue classes in an image with the paintbrush or polygon tools. The mode
    works with multi-component data and multiple image layers.

 -  Redesigned the semi-automatic segmentation GUI to be simpler to use. Now
    the presegmentation can be done without bringing up a separate dialog.
    Also the speed image is immediately computed for most presegmentation
    modes. Overall, semi-automatic segmentation should be much easier to use
    than in the past.

 -  ITK-SNAP can now read 4D datasets. Previously such datasets would have to
    be converted to a multi-component 3D dataset by the user. Now working with
    dynamic datasets is much easier.

 -  Added a label palette control for faster selection of labels

 -  Added a 'flip' button for the 3D scalpel tool

 -  Added support for syncing camera state between multiple SNAP sessions

 -  Significantly improved behavior of open/save dialogs throughout the code.
    All dialogs now use the same code and open in a sensible place.

 -  Added option to auto-adjust image contrast on load

 -  Improved volumes & statistics computation speed and display formatting


### Bug Fixes

 -  Fixed a bug with small bubbles not growing in snake mode.

 -  Refactored the DICOM input code and fixed several errors in the process.

 -  Fixed numerous problems on Retina displays

 -  Modified the clustering code to use Eigendecomposition for increased
    robusness, particularly when applied to binary and multi-label images.

 -  Fixed several dozen smaller bugs


### Programmatic Improvements

 -  Migrated to Qt5 as well as up to date versions of ITK (4.5) and VTK (6.1)

 -  Added scripted GUI testing functionality. This allows interactions to be
    recorded and played back on different platforms and is great for
    regression testing. Four tests are in this release, and more will be
    developed soon.



## Version 3.0.0
----
This is a major new release of ITK-SNAP. The user interface has been completely
rewritten using the Qt platform, and new functionality for multi-modal image
segmentation has been added.

### New functionality for multi-modality image segmentation

 -  SNAP is no longer limited to just scalar-valued and RGB-valued images. An
    image with any number of components can be loaded into SNAP. The GUI provides
    widgets for selecting the currently shown component, deriving scalar images
    from the components (magnitude, maximum, average), and for three-component
    images, rendering them as color RGB images. It is also possible to animate
    components, e.g., for time-varying image data. Multi-component images enjoy
    access to the same features as scalar images, such as curve-based contrast
    adjustment, colormaps, etc.

 -  When multiple layers are loaded into SNAP, the user has a new option to
    tile the layers in each of the 2D slice views. This greatly simplifies
    working with multiple overlays. This also carries over to the automatic
    segmentation mode, where the speed image can now be displayed side by side
    with the anatomical images. It can also aid manual segmentation of
    multi-modality data. During manual segmentation, polygon outlines are traced
    on top of each of the tiled views.

 -  Automatic segmentation (using active contours) can now be performed in
    multi-modality images. Multiple image layers, each of which may have
    multiple components (e.g., RGB, complex or tensor data), can be passed to
    the auto-segmentation mode. Once there, the user can use the new clustering
    preprocessing mode to derive a speed image from this multi-variate input.
    The current implementation of clustering uses Gaussian Mixture Modeling.
    The user selects the desired number of clusters (i.e., tissue classes) and
    once the clusters are initialized, chooses the cluster of interest.

### New features in the Qt-based GUI

 -  The graphical user interface (GUI) uses Qt, a much more powerful toolkit
    than the FLTK toolkit in the previous versions. The new GUI is much richer
    with multiple access paths to common functions (such as choosing the active
    label or changing the color map for an overlay). There are fewer 'apply'
    buttons to press, as most of the time, the program reacts immediately to
    user input into the widgets. More features are available in the left-side
    panel, and these features are organized more logically than before.

 -  New functionality for saving and opening workspaces. A workspace represents
    the state of ITK-SNAP at a given moment, including all the images currently
    loaded in an ITK-SNAP window, as well as associated settings and parameters.
    Workspaces are saved in the XML format. They can be packaged together with
    the images to which they refer and shared with other users.

 -  The layer inspector dialog is greatly improved, with new features for
    reordering layers, quickly changing their visibility, applying a colormap,
    adjusting contrast, saving, etc. The speed image and level set image,
    created by the program during automatic segmentation, are now accessible
    in the layer inspector, so the user can change their color maps as well.
    In the future, the layer inspector will provide access to much more
    functionality, such as applying image processing operations (smoothing,
    feature extraction, bias field correction) to individual layers.

 -  The various plots in the GUI now use the vtkChart library in VTK. This
    provides richer visualization capabilities than the old version in which
    all the plots were rendered using custom OpenGL code. This is particularly
    noticable in the contrast adjustment page of the layer inspector and in
    the preprocessing dialog in auto-segmentation mode.

 -  The window shown at startup shows a graphical list of recently opened images
    and workspaces. This makes it easier to quickly load an image, and keeps the
    GUI clean during startup.

 -  SNAP recognizes pinch gestures (tested on the Mac) for zoom. This should
    make interaction easier for trackpad users.

 -  Layers can be assigned nicknames, such as "T1".

 -  Unicode support. Filenames and user-entered data can now be in any language.

 -  The label editor has new features, such as resetting all labels do defaults,
    filtering labels by name, assinging foreground/background labels directly
    from the dialog.

### Improvements to 3D rendering window

 -  The 3D rendering window now uses the VTK toolkit for rendering. This will
    make it easier to introduce new functionality (such as volume rendering)
    in future versions.

 -  The 3D rendering pipeline is much smarter than before. It detects changes
    to individual labels in the segmentation, so each paint operation no longer
    requires the entire set of 3D meshes to be recomputed. Rendering is
    significantly faster than before.

 -  There is a new option to automatically render meshes in a background thread.
    When enabled, the mesh updates itself in response to polygon and paintbrush
    operations. This works well even for large and complex segmentations. However,
    this is still an experimental feature and may lead to occasional weird crashes
    due to multi-threading issues.

 -  The scalpel tool uses VTK's 3D cutplane widget that can be rotated and moved
    after the cut has been drawn.

### Other new features

 -  Reduced memory footprint for large images. The previous version of SNAP
    would allocate on the order of 6 bytes for every voxel in the main image.
    Two bytes were used to store the grayscale image intensity, two for the
    segmentation, and two for the segmentation undo buffer. The undo buffer is
    now stored in a compressed format, reducing the required memory by almost
    one third. In the future, we also plan to compress the segmentation itself,
    which will cut the memory use by another 2 bytes per voxel.

 -  Improved support for reading/parsing DICOM data. When the user opens a file
    in a directory containing DICOM images, SNAP parses this directory
    much faster than in previous versions (especially when data is on CDs) and
    lists all the series with their dimensions and other meta-data, making it
    easier to determine which series one wishes to load.

### Programmatic improvements

 -  The SNAP code has been extensively refactored. There is a new "model" layer
    separating the Qt GUI from the "logic" layer. This layer is agnostic to the
    type of GUI toolkit used, and implements generic GUI logic. This design
    minimizes the amount of Qt code, so that swapping Qt versions or even
    porting to a different toolkit will be easier in the future. Unlike the old
    FLTK code, which had huge numbers callbacks, the new code relies on a
    widget-model coupling mechanism. This makes the code more robust and
    reduces the amount of Qt-aware code.


## Version 2.4.0
----
This is the last planned release of the FLTK-based version of ITK-SNAP. It adds
minimal new functionality and addressed a number of bugs reported in the last
year. The subsequent releases of ITK-SNAP will be based on the Qt platform and
will have the 3.x version number.

### New Features and UI Improvements

 -  Ported the dependency on ITK 3.20.1 to ITK 4.2 on all operating systems:
    Mac OS X, Linux, and Windows.

 -  ITK-SNAP can read and write MRC images now.

### Bug Fixes and Stability Improvements

 -  Fixed a problem with the RAI code which was not updating after reorienting.

 -  Fixed bug ID: 3371200: The saving of the preprocessed image was failing.

 -  Fixed bug ID: 3309784 and 3415653: Windows file browser broken and enable
    all document setting not available in version 2.2.0.

 -  Fixed bug ID: 3415681 It was not possible to update the mesh when working
    with volumes of one slice. An exception is thrown and catch with fl_alert.

 -  Fixed bug ID: 3323300 BYU mesh saving was save-able, while the internal
    data was not prepared  for this saving. The user interface is correctly
    updated now. In addition, the BYU writer save geometric data as well.

 -  Fixed: bug ID: 3023489: When running from command line with -o flag,
    there was no check to see if images are same size. Two asserts were
    changed in exception throwing.

 -  Fixes for building with different releases of fltk 1.3.

 -  Corrected a bug in the code with SparseLevelSet filter being used
    instead of ParallelSparse.

### Version 2.2.0
----
This is largely a maintenance release, with a few usability enhancements based
on user feedback. The main change programmatically is 64 bit support on Linux,
MacOS and Windows.

### New Features and UI Improvements

 -  64 bit versions of the software are available for Linux, Windows and Mac.
    These versions are now built nightly and will be distributed on
    SourceForge.net. For this to work, we had to change to newer versions of
    the supporting libraries: ITK 3.20, VTK 5.6.1, and FLTK 1.3.0.rc3. The
    latter was necessary for 64 bit MacOS, which many users have requested.
    Thanks to Michael Hanke for providing a patch for ITK 3.18 compatibility.

 -  The maximum number of labels has been increased to 65535 to support
    interoperability with tools like FreeSurfer, which generate segmentations
    with large numbers of labels.

 -  A new window for displaying volumes and statistics. Previously, users had
    to export volumes to a text file in order to view them. Now they can be
    viewed dynamically. This was possible by moving to FLTK 1.3, which
    includes the Fl_Table widget.

 -  A new tab on the layer inspector displaying image metadata, particularly
    useful for DICOM files.

 -  Several changes to the polygon drawing interface. The buttons at the
    bottom of the slice window are now shown dynamically, based on what the
    user is doing. Right clicking brings up a popup menu, allowing to bypass
    the edit mode if desired. An 'undo point' operation is provided.

 -  Users can change the appearance of the polygon drawing UI elements. This
    addresses the request to get rid of the dotted line closing the polygon.

 -  Intensity window and level in the image contrast dialog are no longer
    clamped by the minimum and maximum intensity in the image. This is useful
    for displaying statistical maps, where a certain fixed output range is
    desired.

 -  Finally implemented all the options under Segmentation->Export as Mesh.
    You can now export meshes for all labels either as separate mesh files or
    as a single scene. The latter is recommended with the VTK mesh format,
    where the label ids of the meshes are preserved.

 -  Collapsable slice windows. The new 'collapse' button gets rid of the UI
    and just shows the selected slice. This is useful when you have multiple
    SNAP sessions open at once. SNAP can be opened in this mode using the new
    command-line option '--compact <axis>', where <axis> is 'a' for axial, 's'
    for sagittal or 'c' for coronal. You can restore default SNAP layout using
    Ctrl-F3 (Command-F3 on the Mac) or using the toolbar button that pops up.

 -  Also added command line options --zoom and --help

 -  The 'reset view' button under the slice windows has been renamed 'zoom to
    fit' and it behaves more sensibly when zoom is linked across the slice
    views.

 -  Improved integration with MacOS and Windows operating systems. On both
    MacOS and Windows, you can drag and drop a file into an open SNAP window
    and you will be prompted to open that file as a grey image, as a
    segmentation, as an overlay, or as a grey image in another SNAP session.
    Additionally, on the Mac, you can drag and drop files to the SNAP icon on
    the Dock, even if an SNAP session is not running.

 -  Added an option under File menu to open a new SNAP session.

 -  Ability to save segmentation mesh in active contour mode

### Bug Fixes and Stability Improvements

 -  Fixed a problem with certain operations being very slow because of the way
    the progress bars were displayed. Preprocessing, mesh rendering and mesh
    IO will now be much faster

 -  Fixed problems with the snake parameter dialog. The images are now
    properly displayed and animation works.

 -  Fixed problems with automatic panning in crosshairs mode. Also added a
    button to enable this feature; it is disabled by default.

 -  Changed defaults for edge-based snakes to have non-zero weight

 -  Fixed display issues on newer MacBook Pro

 -  Fixed problem with bubbles not being spherical for certain image orientations

## Version 2.0
----
### New Features and UI Improvements

 -  Support for multiple image layers. Users can now load gray and RGB images
    as overlays on top of the main image layer. For example, one can display a
    statistical map as an overlay over an anatomical image. As of version
    1.9.8, overlays must have the same dimensions as the main image.

 -  A new layer inspector window. Each layer in SNAP (main image and each of the
    overlays) can be examined using the layer inspector. Currently there are three
    tabs: one for setting the intensity mapping of the layer (i.e., mapping from
    image intensity to display intensity); one for selecting and editing the color
    map and transparency of the layer; and one providing information about the layer.
    The layer inspector replaces the old "Image Information" and "Intensity Curve"
    windows. The color bar editor is only partially functional as of 1.9.9.

 -  Hiding the UI. Using the 'F3' key, users can toggle certain user interface elements
    on and off. Press 'F3' once, and the left sidebar and the menu bar disappear.
    Press 'F3' twice, and all the UI elements disappear, so you are looking just at the
    image. Press 'F3' again, and the UI is restored to the original state. This
    feature works well with the '+' buttons on the slice windows. It's intended for
    multi-session SNAP users, so that the screen real estate can be used more efficiently
    by multiple SNAP sessions.

 -  Because now the most common SNAP commands have a shortcut, you will be able to
    do a lot with the UI hidden. Select 'Help->Keyboard Shortcuts' to see a listing.

 -  Fullscreen mode. Press 'F4' to toggle fullscreen SNAP. Use it with 'F3' to let the
    image occupy the whole screen.

 -  An expanded menu bar. We have split the menu into File, Segmentation and Overlay
    menus to provide easier and faster access to the ITK-SNAP features.

 -  Native file chooser. On Windows and MacOS, ITK-SNAP will use a native file
    chooser instead of the FLTK built-in file chooser. On Mac OSX, the native
    file chooser can be further enhanced by installing the DTI-TK Quick Look
    plugin that supports NIfTI/Analyze image preview (www.nitrc.org/projects/dtitk)

 -  When launched from command line, SNAP can automatically determine whether an
    image is a 3-component RGB image or a grayscale image. To use this functionality,
    users must run SNAP without "-g" or "-rgb" options:

        itksnap image.nii

    This feature is ideal for users who want to associate ITK-SNAP with certain 3D image
    types in their operating system (in Finder or Windows Explorer).

 -  Automatic check for software update. Users can enable automatic update checking.

 -  External web browser support. Help and other HTML pages are now displayed in the
    operating system's own web browser, from itksnap.org. This may displease users
    connected to the internet, but this makes managing documentation a lot easier and
    hopefully will allow us to keep the documentation up to date with the features.

 -  Crash recovery. When an out-of-memory or other crash occurs, ITK-SNAP will ask you
    if you want to save the segmentation image before exiting. Of course this may not
    always work, but it should make a lot of frustrated users a little less frustrated.

 -  Reduced the memory footprint. There is still room for improvement, of course. Currently, ITK-SNAP requires 6 bytes per voxel in manual segmentation mode. More memory is needed for mesh rendering, and a lot more for automatic segmentation. When loading images in 32-bit or 64-bit formats, more memory may be required at the time of image IO. That is because ITK NIFTI reader (and maybe other readers) keeps a second copy of the image in memory during IO. This memory is immediately deallocated though.

 -  Unified navigation modes. The crosshair mode allows zoom and pan (RMB/MMB), and has an auto-pan feature when you move the crosshair close to the edge of the slice window. The zoom/pan mode is redundant, but we left it in place for backward compatibility. In the zoom/pan mode, zoom is RMB, pan is LMB, crosshair motion is MMB. In all other modes, crosshair motion is accessible through MMB as well.

### Bug Fixes

 -  Fixed an issue with SNAP reading certain types of image twice from disc. This should
    speed up the reading of floating point images, for example.

 -  Color map cache is now computed on the fly. This makes interaction with the intensity
    curve and color map more real-time.

 -  Found a problem where in Release mode, the active contour would do
    nothing. Did not know how to fix it correctly, so replaced the parallel
    sparse field solver with the non-parallel one. This may slow down
    automatic segmentation on some machines, so this is an outstanding issue.

 -  Found a bug that caused images with unusual coordinate orientations to be incorrectly
    displayed (wrong coordinate labels assigned). This was caused by incorrect mapping of
    ITK direction matrix to "RAI" codes in SNAP. This affects display of NIFTI, DICOM and
    other image files. It also affects the behavior of the Reorient Image dialog.

 -  Please see the bug tracker on itksnap.org for the full listing of bug fixes.

### Website Changes

 -  The itksnap.org website has been Wikified. Content can now be edited on the fly.



## Version 1.8
----
### New Features and UI Improvements

 -  Support for reading floating point images of arbitrary range. SNAP still
    represents gray images internally as signed short, but now it can load a
    floating point image and remap its intensity range to signed shorts. When
    displaying intensity values, it will map back to float.

 -  A new 'Adaptive' paintbrush. Under the paintbrush tool, it can be selected
    using the 'Shape' drop down.  This tool can speed up manual segmentation
    quite a bit for some users. This brush has the shape of a rectagle. As you
    click on a pixel in one of the slice views, the brush will fill a region
    that includes the pixel you clicked and has more or less uniform intensity.
    For example, in brain MRI, if you click in the ventricles near the caudate,
    the brush will fill the ventricle but not the caudate. This is not as
    powerful as running the level set segmentation, but it's very local and
    great for quickly segmenting structures - or dealing with inhomogeneities.
    The underlying algorithm is ITK's watershed segmentation. You can control
    the tolerance of the adaptive brush ('granularity' input, lower values
    produce smaller, more cohesive regions). The brush can be used in 2D or 3D.

    This feature was inspired by a similar tool in ITKGrey, a tool from the
    Vista Lab at Stanford that itself is a branch of an older version of
    ITK-SNAP. Let us know if this feature works for you. Potentially, we may add
    other algorithms in the future, including running the level set inside of
    the brush.

 -  Support for image orientation. This is a major step towards NIFTI
    compatibility (part of our R03 effort) and something many users should find
    helpful. Formats such as NIFTI, DICOM, and a couple others encode the
    orientation of the image axes in patient space, and even allow image axes to
    not be parallel to the anatomical axes. SNAP now reads this information from
    the image header and uses it to assign anatomical labels and compute
    anatomical coordinates. One of the consequences of this change is that the
    image IO wizard no longer requires specifying an orientation code (e.g.,
    'RAI') when loading an image, since this information is read in the header.

 -  A new 'reorient image' dialog has been added, so that if the orientation
    information in the header is wrong, you can change the orientation and save
    the image. For now, the user can only specify reorientations that are
    parallel to the anatomical axes.

 -  World cursor coordinates (under image information) are now displayed in
    NIFTI / MNI coordinates as well as ITK coordinates. The difference is that
    the NIFTI coordinates incorporate orientation and are in the (L->R, P->A,
    I->S) coordinate frame. ITK coordinates are (x * spacing + origin), and
    ignore orientation.

 -  3D Meshes generated and rendered by SNAP are now represented in NIFTI world
    coordinates. Previously, the coordinates were computed using the formula

        x_mesh = x_voxel * spacing + origin

    In version 1.8 and beyond, the mesh coordinates are computed as

        x_mesh = nifti_sform_matrix * [x_voxel; 1]

    This means that the meshes output by earlier versions of SNAP may be
    translated and rotated relative to the meshes output by version 1.8. This will
    not affect users who simply view meshes in SNAP; however users who export
    meshes to other programs will be affected.

 -  Multisession cursor (similar to yoking in MRIcro) now uses these NIFTI
    coordinates rather than ITK coordinates. This is a key feature because it
    enables users to work with MRI scans acquired during the same session with
    different orientations. For example, a coronal T1 scan and an oblique T2
    scan can be loaded in two SNAP instances, and the cursor will be correctly
    linked across the two.

    CAVEAT: SNAP's cursor always falls on voxel centers. This means that the
    multisession cursor correspondence is not exact, but rounded to the nearest
    voxel. If in session A you move your cursor, the cursor in session B will
    move to the voxel center closest to the physical position referenced by the
    cursor in session A.

 -  A new multi-session zoom feature. Similar to the multi-session cursor, this
    allows the zoom level to be maintained across multiple SNAP sessions. Useful
    if you do a lot of zooming in and out when working with a pair of scans.
    This is disabled by default and must be enabled in each SNAP session using
    the checkbox under the 'Zoom/Pan Tool'.

 -  Changes to how zoom works, related to above. Now 'zoom views together' is on
    by default, meaning that the zoom factor is the same in axial, coronal and
    sagittal windows. Zoom level is specified in px/mm, where px is the number
    of screen pixels (in other words, a metric equivalent of dots per inch).
    Before, zoom was specified in percent, relative to an optimal zoom that
    would best fit all three windows. With the new way you have more control
    over the zoom. For example, if your image has 1mm voxels, you can have one
    to one correspondence between screen pixels and voxels by setting the zoom
    to 1 px/mm.

 -  Multisession 3D views. When the multisession cursor is selected, the 3D
    views are also synchronized across sessions. This works even if the images
    opened in the two SNAP sessions have different dimensions, orientation and
    spacing. SNAP 3D window now uses NIFTI world coordinates, so as long as the two
    images overlap in world space, so will the 3D views of the two images. This
    feature is useful when comparing two segmentations of the same image.

 -  A new ruler display in slice windows. Can be disabled or modified on the
    display options dialog.

 -  Much better tracking of changes to the segmentation image and better
    promting to save changes before quitting or loading a new image. The title
    bar display is also improved and uses an asterisk to indicate unsaved
    changes.

 -  The command-line options have been updated. You can now load a grey image
    without using any flags (e.g., itksnap image.nii) and there is a new '-rgb'
    flag for loading an RGB image from command line. The upshot is that you can
    now associate SNAP with image file extensions in the operating system and
    double-click an image file to open it in SNAP.

 -  A new 'Tools' dialog on the label editor. This dialog is intended to provide
    several tools for merging or modifying labels. The first tool is to combine
    a pair of labels into one. Previously, this was possible using the 3D
    scalpel tool, but that was not really an intuitive way to relabel images.

 -  As part of above, a new topological merge tool, developed by Nick Tustison,
    Brian Avants and Marcelo Siqueira (I hope I did not forget anyone). Given
    adjacent labels A and B, it will replace most voxels in B with the label A,
    while preserving the topology of A. This tool is used to preserve topology
    during manual segmentation. If A has correct topology and you want to add
    some region to A, label this region with label B, and then grow A into B
    with topology preservation. This is a work in progress, and feedback would
    be welcome on this feature.

 -  Documented existing keyboard shortcuts and added some new ones. Available
    shortcuts can be listed by selecting Help->Shortcuts.

### Programmatic/Distribution Changes

 -  SNAP is now built against ITK 3.8, offering several improvements, especially
    in how image orientation is handled.

 -  IPC communications (technology that allows multisession cursor and zoom) now
    has some versioning built into it, so if you are running two versions of
    SNAP, they will not clash.

 -  On LINUX, we now distribute a .tgz archive instead of a script installer.
    Some people complained about the latter. We can also make .rpm and .deb
    packages although these won't be posted for public download yet.

1.7.3. Bug Fixes

 -  Level set fix for ITK 3.8 fixes automatic segmentation's weird behavior

## Version 1.6.0.1
----
### Bug Fixes

 -  Major bug in release 1.6.0 involving disabled cursor movement in snake
    segmentation mode has been resolved.


## Version 1.6.0
----
### New Features and UI Improvements

 -  You can now save a sequence of all axial, coronal or sagittal slices with
    overlays as PNG files (File->Save->Screenshot Series).

 -  Automatic window and level computation based on the image histogram. The
    window and level are set to the 1st and 99th percentiles of the intensity
    histogram, respectively. This is much more robust to hypo and
    hyper-intensity in medical imaging data. The feature is accessed in the
    "Options->Image Contrast" menu (or hit Alt-I in the main window).

 -  Cursor synchronization across multiple SNAP sessions (similar to the Yoke
    feature in MRIcro). The mechanism uses POSIX shared memory. Can be turned
    off using the 'Synchronize Cursor' checkbox. Currently, only enabled in
    manual segmentation mode; probably will enable in snake mode in the near
    future.

        --- NOTE FOR MacOS Users ---
        MacOS doesn't allow you double-click the application icon to open a
        new instance. To open multiple instances of ITK-SNAP, you need to launch
        it from the command line.
        ----------------------------

 -  SNAP will prompt you before closing if there are unsaved changes.

 -  A new 'New->Segmentation Image' menu item will clear the current
    segmentation.

 -  Support for RGB (color) images in SNAP. This is great for segmenting in DTI
    data (manually, for the time being). RGB images can be loaded as the base
    image or as an overlay over the gray. To create these RGB images, use the new
    DTI-TK developed by Hui (Gary) Zhang, available from

        http://www.picsl.upenn.edu/resources_dtitk.aspx

 -  Segmentations can be exported as VTK meshes (for example, for loading in
    ParaView).

 -  Multilevel undo/redo functionality for all segmentation operations (polygon,
    paintbrush, freehand, 3D segmentation, 3D cutplane). Undo memory is
    preserved when loading new segmentation images.

 -  Freehand drawing support in polygon mode (hold and drag the mouse button).
    This feature is especially useful for using SNAP on a tablet.

 -  Added keyboard shortcuts 'a','s','d' for the opacity slider

 -  Shortened/simplified some of the menu items

### Bug Fixes

 -  Various bugs have been fixed :)

### Distribution Changes

 -  SNAP website fully migrated to sourceforge.net

 -  Mac Universal binaries supporting Intel and PCC, Tiger and Leopard are now
    available starting with 1.6.0

 -  Linux binaries will be available starting with 1.6.0


## Version 1.4.1
----
### New Features and UI Improvements

 -  Added paintbrush tool to the main toolbar. Paintbrush can be used to quickly
    touch up segmentations. Left mouse button paints with selected label, right
    button acts as an erasor

 -  Went through and added/edited tooltips in the program to be more accurate. It
    should be easier to make sense of the program now

 -  Added a menu option for saving the level set image during active contour
    evolution. This is an important feature because it allows users to save
    segmentations before sub-voxel accuracy is lost. In particular, this can be
    used in conjunction with ParaView to generate meshes from segmentations.

 -  You can now save and restore the camera settings in the 3D view within a
    single SNAP session. This can be useful for generating screen shots of
    different segmentation from the same viewpoint. Press 's' in the 3D window
    to save the camera state and 'r' to restore it.

### Bug Fixes

 -  MAJOR: fixed bug that was causing crashes on Win32 during polygon drawing
    (thanks to Jeff Tsao for this bug fix!)

 -  Fixed problems with the getsnap.sh linux script

 -  Some menu items were enabled when they should not have been, now are
    disabled.

 -  Rare bug where speed function very close to 1 was not being rounded
    correctly and may have caused crashes on some systems

 -  Fixed problem where the screen was blank after loading preprocessed image

 -  Fixed crash when changing bubble radius and then going back to
    preprocessing mode

### Distribution Changes

 -  Interim SNAP releases are now hosted on SourceForge. ITK repository will only
    be used to host major releases (like 1.6). This allows us to check stuff in
    independently of the ITK code freezes. It also makes it easier to add new
    developers.

 -  SNAP CMake files should automatically detect when SNAP is being built
    outside of ITK's InsightApplications. This means you can build SNAP on it's
    own and the download size is reduced


## Version 1.4
----
### New Features and User Interface Improvements

 -  New and improved label editor. You can easily switch between labels while in
    the editor and the interface for adding new labels is more intuitive. You
    can now delete labels.

 -  New and improved interface for intensity reparameterization. The histogram
    display is more visible and you have more control over the number of bins in
    the histogram and the scaling of the bars (linear or log).

 -  SNAP remembers all settings associated with loading an image. This means that
    any image loaded previously can be reloaded without going throught the
    wizard.

 -  We've added File->Load Previous menu to let you load images quickly

 -  SNAP can now read DICOM file series (experimental support) and it can read
    and write VoxBo CUB image files.

 -  SNAP remembers more image-associated settings from session to session. For
    example, it will remember the intensity reparameterization that you last
    used. SNAP will also remember the orientation ("RAI" code) that was last
    used to read each image.

 -  New Image Information window is available under the File menu. It displays
    the size of the image and the current cursor position.

 -  A color map feature has been added in the automatic segmentation mode. The
    color map lets you select different color schemes for displaying the
    probability map / speed image.

 -  Small improvements to the active contour 2D example dialog have been made

 -  A progress monitor has been added for the 3D renderer in main SNAP window.

 -  New buttons allow taking of snapshots in each of the SNAP image windows

 -  The tutorial has been updated to reflect the new features.

### Bug Fixes.

 -  SNAP should crash a lot less than before

 -  The Left-Right orientation should be correctly handled by SNAP. You still
    have to supply the correct orientation ("RAI Code") when loading the image.

 -  The bug with the segmentation being shifted when using "Resample Region"
    option has been fixed

 -  3D window handles images with non-zero origin better

 -  Initialization bubbles have been fixed to be floating point

 -  Lots of other small bugs have been fixed!

### Programmatic Enhancements

 -  SNAP and IRIS now share the sameset of OpenGL windows. This should prevent
    crashes on some platforms.

### Other

 -  SNAP available as a universal (Intel/PPC) binary for MacOS at itksnap.org


## Version 1.2
----
### User Interface Improvements

 -  The ability to switch between 4-view mode and single view mode. Each of the
    slice views and the 3D view can be expanded to occupy the entire SNAP window.

 -  A zoom thumbnail is now displayed when a slice view is zoomed in. The thumbnail
    view can be used to pan the slice.

 -  User can specify whether he/she prefers to start in linked zoom mode or in
    unlinked zoom mode.

 -  User can change the appearance of various display elements, including the
    crosshairs, the region of interest selection box, the window background and
    more.

 -  SNAP automatically determines the image orientation (RAI) when that information
    is available in the image file

 -  SNAP remembers the last ROI used for each image.

### Programmatic Improvements

 -  The level set segmentation pipeline has been rewritten, taking advantage of
    the stop and go functionality of ITK finite difference filters. This means
    fewer unexplained crashes and simpler code.

 -  A state-machine has been added to the user interface logic code. This
    machine automatically activates and deactivates UI widgets based on a set of
    flags. Rules such as Flag A => Flag B can be added to the state machine.

### Bug Fixes

 -  Slice views update correctly when the SNAP window is resized

 -  Accepting a polygon now works for high resolution images.

 -  Fixed a crash on some systems when running edge-based snake segmentation with an
    advection term.



