#ifndef UISTATE_H
#define UISTATE_H

/** A list of states that the Global UI may be in. Whenever any of these
  states changes, the UI issues a StateChangedEvent */
enum UIState {
  UIF_BASEIMG_LOADED,            // i.e., Gray or RGB loaded
  UIF_OVERLAY_LOADED,            // i.e., Baseimg loaded and at least one overlay
  UIF_IRIS_MODE,                 // Not in snake mode (or other future such mode)
  UIF_IRIS_WITH_BASEIMG_LOADED,  // i.e., system in main interaction mode
  UIF_IRIS_WITH_OVERLAY_LOADED,  // i.e., system in main interaction mode
  UIF_ROI_VALID,
  UIF_LINKED_ZOOM,
  UIF_UNDO_POSSIBLE,
  UIF_REDO_POSSIBLE,
  UIF_UNSAVED_CHANGES,
  UIF_MESH_SAVEABLE,
  UIF_SNAKE_MODE,
  UIF_LEVEL_SET_ACTIVE,
  UIF_MULTIPLE_BASE_LAYERS       // i.e., more than one non-sticky layer
};

#endif // UISTATE_H
