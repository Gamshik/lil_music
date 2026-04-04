export type TabId = "home" | "search" | "queue" | "equalizer";

export type PlaybackStatus = "idle" | "loading" | "playing" | "paused" | "error";

export type RepeatMode = "none" | "playlist" | "track";

export type EqualizerStatus =
  | "loading"
  | "ready"
  | "unsupported_audio_path"
  | "audio_engine_unavailable"
  | "error";

export type EqualizerPresetId =
  | "flat"
  | "bass_boost"
  | "treble_boost"
  | "vocal"
  | "pop"
  | "rock"
  | "electronic"
  | "hip_hop"
  | "jazz"
  | "classical"
  | "acoustic"
  | "dance"
  | "piano"
  | "spoken_podcast"
  | "loudness"
  | "custom";

export interface Track {
  id: string;
  title: string;
  artistName: string;
  artworkUrl: string;
  streamUrl: string;
}

export interface AppInfo {
  ok: true;
  applicationName: string;
  bridgeStatus: string;
  playbackBackend: string;
}

export interface AudioOutputDevice {
  id: string;
  displayName: string;
  isDefault: boolean;
  isSelected: boolean;
}

export interface AudioOutputDevicesResponse {
  ok: true;
  devices: AudioOutputDevice[];
}

export interface QueueState {
  currentTrackId: string;
  currentTrackTitle: string;
  queuedTracks: Track[];
  shuffleEnabled: boolean;
  repeatMode: RepeatMode;
  canPlayPrevious: boolean;
  canPlayNext: boolean;
}

export interface QueueStateResponse extends QueueState {
  ok: true;
}

export interface PlaybackState {
  state: PlaybackStatus;
  streamUrl: string;
  errorMessage: string;
  positionMs: number;
  durationMs: number;
  volumePercent: number;
  completionToken: number;
  trackId: string;
  trackTitle: string;
  shuffleEnabled: boolean;
  repeatMode: RepeatMode;
  canPlayPrevious: boolean;
  canPlayNext: boolean;
}

export interface PlaybackStateResponse extends PlaybackState {
  ok: true;
}

export interface SearchResponse {
  ok: true;
  query: string;
  limit: number;
  offset: number;
  tracks: Track[];
}

export interface FeaturedTracksResponse {
  ok: true;
  title: string;
  limit: number;
  tracks: Track[];
}

export interface SimpleTrackCommandResponse {
  ok: true;
  trackId?: string;
  trackTitle?: string;
  state?: PlaybackStatus;
  advanced?: boolean;
}

export interface SeekResponse {
  ok: true;
  positionMs: number;
  trackId: string;
}

export interface VolumeResponse {
  ok: true;
  volumePercent: number;
}

export interface EqualizerBand {
  index: number;
  centerFrequencyHz: number;
  gainDb: number;
}

export interface EqualizerPreset {
  id: EqualizerPresetId;
  title: string;
  isDerived: boolean;
  gainsDb: number[];
}

export interface EqualizerState {
  status: EqualizerStatus;
  enabled: boolean;
  activePresetId: EqualizerPresetId;
  outputGainDb: number;
  headroomCompensationDb: number;
  errorMessage: string;
  bands: EqualizerBand[];
  presets: EqualizerPreset[];
}

export interface EqualizerStateResponse extends EqualizerState {
  ok: true;
}

export interface BridgeErrorResponse {
  ok: false;
  message: string;
}

export type BridgeResponse<T> = T | BridgeErrorResponse;
