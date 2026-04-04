import type {
  EqualizerState,
  PlaybackState,
  QueueState,
  Track,
} from "@models/models";

export const EMPTY_TRACKS: Track[] = [];

export const DEFAULT_PLAYBACK_STATE: PlaybackState = {
  state: "idle",
  streamUrl: "",
  errorMessage: "",
  positionMs: 0,
  durationMs: 0,
  volumePercent: 100,
  completionToken: 0,
  trackId: "",
  trackTitle: "",
  shuffleEnabled: false,
  repeatMode: "none",
  canPlayPrevious: false,
  canPlayNext: false,
};

export const DEFAULT_QUEUE_STATE: QueueState = {
  currentTrackId: "",
  currentTrackTitle: "",
  queuedTracks: [],
  shuffleEnabled: false,
  repeatMode: "none",
  canPlayPrevious: false,
  canPlayNext: false,
};

export const DEFAULT_EQUALIZER_STATE: EqualizerState = {
  status: "loading",
  enabled: false,
  activePresetId: "flat",
  outputGainDb: 0,
  headroomCompensationDb: 0,
  errorMessage: "",
  bands: [],
  presets: [],
};
