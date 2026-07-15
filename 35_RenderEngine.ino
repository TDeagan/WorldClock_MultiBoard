// ============================================================
// World Clock rendering engine
// ============================================================
//
// Full-display layer order:
//
//   1. Earth map and day/night terminator
//   2. Optional latitude/longitude grid
//   3. ISS orbit track
//   4. Sun
//   5. Moon
//   6. Current ISS marker
//   7. Optional home-location marker
//   8. Clock/status bar and IP address
//
// The scheduler and recovery code call only redrawWorldClock() or
// renderStatusBar(); they no longer need to know the layer order.
// ============================================================

RenderState renderState;


void renderOverlayLayers(time_t epoch) {
  renderCoordinateGridOverlay();
  renderIssTrackOverlay();
  renderCelestialOverlays(epoch);
  renderIssOverlay();
  renderHomeLocationOverlay();

  renderState.overlayLayersDrawn = true;
}


void renderStatusBar() {
  drawClock();

  renderState.epoch =
    time(nullptr);

  renderState.statusLayerDrawn = true;
}


bool renderWorldDisplay() {
  if (
    !timeValid ||
    !sdReady
  ) {
    return false;
  }

  renderState.epoch =
    time(nullptr);

  renderState.fullRedrawInProgress = true;
  renderState.baseLayerDrawn = false;
  renderState.overlayLayersDrawn = false;
  renderState.statusLayerDrawn = false;

  if (
    !renderBaseMapLayer(
      renderState.epoch
    )
  ) {
    renderState.fullRedrawInProgress = false;
    return false;
  }

  renderState.baseLayerDrawn = true;

  renderOverlayLayers(
    renderState.epoch
  );

  renderStatusBar();

  renderState.fullRedrawInProgress = false;
  lastMapUpdate = millis();

  return true;
}


void redrawWorldClock() {
  if (
    !timeValid ||
    !sdReady
  ) {
    return;
  }

  if (!renderWorldDisplay()) {
    sdReady = false;

    setSystemError(
      SystemError::SdUnavailable,
      "Map scanline read failed"
    );
  }
}
