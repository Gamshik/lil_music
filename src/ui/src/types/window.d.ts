import type {
  AppInfo,
  AudioOutputDevicesResponse,
  EqualizerStateResponse,
  FeaturedTracksResponse,
  PlaybackStateResponse,
  QueueStateResponse,
  SearchResponse,
  SeekResponse,
  SimpleTrackCommandResponse,
  VolumeResponse,
} from "./models";

declare global {
  interface Window {
    getAppInfo?: () => Promise<AppInfo | string>;
    getEqualizerState?: () => Promise<EqualizerStateResponse | string>;
    getPlaybackState?: () => Promise<PlaybackStateResponse | string>;
    getAudioOutputDevices?: () => Promise<AudioOutputDevicesResponse | string>;
    getQueueState?: () => Promise<QueueStateResponse | string>;
    getFeaturedTracks?: (payload: unknown) => Promise<FeaturedTracksResponse | string>;
    enqueueTrack?: (payload: unknown) => Promise<QueueStateResponse | string>;
    removeQueuedTrack?: (payload: unknown) => Promise<QueueStateResponse | string>;
    playTrack?: (payload: unknown) => Promise<SimpleTrackCommandResponse | string>;
    playPreviousTrack?: () => Promise<SimpleTrackCommandResponse | string>;
    playNextTrack?: () => Promise<SimpleTrackCommandResponse | string>;
    playCompletionFollowUp?: () => Promise<SimpleTrackCommandResponse | string>;
    toggleShuffle?: () => Promise<QueueStateResponse | string>;
    cycleRepeatMode?: () => Promise<QueueStateResponse | string>;
    pausePlayback?: () => Promise<SimpleTrackCommandResponse | string>;
    resumePlayback?: () => Promise<SimpleTrackCommandResponse | string>;
    seekPlayback?: (payload: unknown) => Promise<SeekResponse | string>;
    selectAudioOutputDevice?: (payload: unknown) => Promise<AudioOutputDevicesResponse | string>;
    setPlaybackVolume?: (payload: unknown) => Promise<VolumeResponse | string>;
    setEqualizerEnabled?: (payload: unknown) => Promise<EqualizerStateResponse | string>;
    selectEqualizerPreset?: (payload: unknown) => Promise<EqualizerStateResponse | string>;
    setEqualizerBandGain?: (payload: unknown) => Promise<EqualizerStateResponse | string>;
    setEqualizerOutputGain?: (payload: unknown) => Promise<EqualizerStateResponse | string>;
    resetEqualizer?: () => Promise<EqualizerStateResponse | string>;
    searchTracks?: (payload: unknown) => Promise<SearchResponse | string>;
    toggleFavorite?: (payload: unknown) => Promise<unknown>;
  }
}

export {};
