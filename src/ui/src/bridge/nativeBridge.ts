import type {
  AppInfo,
  AudioOutputDevicesResponse,
  BridgeErrorResponse,
  BridgeResponse,
  EqualizerStateResponse,
  FeaturedTracksResponse,
  PlaybackStateResponse,
  QueueStateResponse,
  SearchResponse,
  SeekResponse,
  SimpleTrackCommandResponse,
  VolumeResponse,
} from "../types/models";

type BridgeFunctionName =
  | "getAppInfo"
  | "getEqualizerState"
  | "getPlaybackState"
  | "getAudioOutputDevices"
  | "getQueueState"
  | "getFeaturedTracks"
  | "enqueueTrack"
  | "removeQueuedTrack"
  | "playTrack"
  | "playPreviousTrack"
  | "playNextTrack"
  | "playCompletionFollowUp"
  | "toggleShuffle"
  | "cycleRepeatMode"
  | "pausePlayback"
  | "resumePlayback"
  | "seekPlayback"
  | "selectAudioOutputDevice"
  | "setPlaybackVolume"
  | "setEqualizerEnabled"
  | "selectEqualizerPreset"
  | "setEqualizerBandGain"
  | "setEqualizerOutputGain"
  | "resetEqualizer"
  | "searchTracks";

function normalizeBridgePayload<T>(payload: T | string): T {
  if (typeof payload !== "string") {
    return payload;
  }

  return JSON.parse(payload) as T;
}

async function callBridge<TResponse>(
  methodName: BridgeFunctionName,
  payload?: Record<string, unknown>,
): Promise<BridgeResponse<TResponse>> {
  const bridgeMethod = window[methodName] as ((payload?: unknown) => Promise<unknown>) | undefined;
  if (typeof bridgeMethod !== "function") {
    throw new Error(`Native bridge method "${methodName}" is unavailable.`);
  }

  const response = payload === undefined ? await bridgeMethod() : await bridgeMethod(payload);
  return normalizeBridgePayload<BridgeResponse<TResponse>>(response as BridgeResponse<TResponse> | string);
}

export const nativeBridge = {
  getAppInfo: () => callBridge<AppInfo>("getAppInfo"),
  getEqualizerState: () => callBridge<EqualizerStateResponse>("getEqualizerState"),
  getPlaybackState: () => callBridge<PlaybackStateResponse>("getPlaybackState"),
  getAudioOutputDevices: () => callBridge<AudioOutputDevicesResponse>("getAudioOutputDevices"),
  getQueueState: () => callBridge<QueueStateResponse>("getQueueState"),
  getFeaturedTracks: (limit: number) =>
    callBridge<FeaturedTracksResponse>("getFeaturedTracks", { limit }),
  enqueueTrack: (trackId: string) => callBridge<QueueStateResponse>("enqueueTrack", { trackId }),
  removeQueuedTrack: (trackId: string) =>
    callBridge<QueueStateResponse>("removeQueuedTrack", { trackId }),
  playTrack: (trackId: string, title: string) =>
    callBridge<SimpleTrackCommandResponse>("playTrack", { trackId, title }),
  playPreviousTrack: () => callBridge<SimpleTrackCommandResponse>("playPreviousTrack"),
  playNextTrack: () => callBridge<SimpleTrackCommandResponse>("playNextTrack"),
  playCompletionFollowUp: () =>
    callBridge<SimpleTrackCommandResponse>("playCompletionFollowUp"),
  toggleShuffle: () => callBridge<QueueStateResponse>("toggleShuffle"),
  cycleRepeatMode: () => callBridge<QueueStateResponse>("cycleRepeatMode"),
  pausePlayback: () => callBridge<SimpleTrackCommandResponse>("pausePlayback"),
  resumePlayback: () => callBridge<SimpleTrackCommandResponse>("resumePlayback"),
  seekPlayback: (positionMs: number) =>
    callBridge<SeekResponse>("seekPlayback", { positionMs }),
  selectAudioOutputDevice: (deviceId: string) =>
    callBridge<AudioOutputDevicesResponse>("selectAudioOutputDevice", { deviceId }),
  setPlaybackVolume: (volumePercent: number) =>
    callBridge<VolumeResponse>("setPlaybackVolume", { volumePercent }),
  setEqualizerEnabled: (enabled: boolean) =>
    callBridge<EqualizerStateResponse>("setEqualizerEnabled", { enabled }),
  selectEqualizerPreset: (presetId: string) =>
    callBridge<EqualizerStateResponse>("selectEqualizerPreset", { presetId }),
  setEqualizerBandGain: (bandIndex: number, gainDb: number) =>
    callBridge<EqualizerStateResponse>("setEqualizerBandGain", { bandIndex, gainDb }),
  setEqualizerOutputGain: (outputGainDb: number) =>
    callBridge<EqualizerStateResponse>("setEqualizerOutputGain", { outputGainDb }),
  resetEqualizer: () => callBridge<EqualizerStateResponse>("resetEqualizer"),
  searchTracks: (query: string, limit: number, offset: number) =>
    callBridge<SearchResponse>("searchTracks", { query, limit, offset }),
};
