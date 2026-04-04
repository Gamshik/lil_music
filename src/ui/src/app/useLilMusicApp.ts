import { startTransition, useEffect, useEffectEvent, useRef, useState } from "react";
import { nativeBridge } from "@bridge/nativeBridge";
import {
  DEFAULT_EQUALIZER_STATE,
  DEFAULT_PLAYBACK_STATE,
  DEFAULT_QUEUE_STATE,
  EMPTY_TRACKS,
} from "@app/defaultState";
import type {
  AppInfo,
  AudioOutputDevice,
  EqualizerPresetId,
  EqualizerState,
  PlaybackState,
  PlaybackStatus,
  QueueState,
  TabId,
  Track,
} from "@models/models";
import {
  getEqualizerPresetTitle,
  mapEqualizerStatusLabel,
  mapPlaybackStateLabel,
} from "@utils/formatters";

interface PlaybackStatusView {
  label: string;
  details: string;
  visualState: PlaybackStatus;
}

function isOfflineMode(): boolean {
  return typeof navigator !== "undefined" && navigator.onLine === false;
}

function hasPlaybackEnded(previousState: PlaybackState | null, nextState: PlaybackState): boolean {
  if (!previousState) {
    return false;
  }

  return (
    nextState.completionToken > previousState.completionToken &&
    Boolean(nextState.trackId || previousState.trackId)
  );
}

function clampVolumePercent(volumePercent: number): number {
  return Math.min(100, Math.max(0, Number(volumePercent) || 0));
}

export function useLilMusicApp() {
  const [bridgeTitle, setBridgeTitle] = useState("Подключение...");
  const [bridgeDetails, setBridgeDetails] = useState("Ожидание native-ответа");
  const [playbackBackend, setPlaybackBackend] = useState("Playback backend: ожидание...");
  const [activeTabId, setActiveTabId] = useState<TabId>("home");
  const [searchQuery, setSearchQuery] = useState("");
  const [searchLimit, setSearchLimit] = useState(24);
  const [searchOffset, setSearchOffset] = useState(0);
  const [searchSummary, setSearchSummary] = useState(
    "Поиск работает через native bridge, но интерфейс остаётся быстрым и простым.",
  );
  const [featuredSummary, setFeaturedSummary] = useState(
    "Стартовая лента подгрузит треки, которые можно слушать сразу.",
  );
  const [searchTracks, setSearchTracks] = useState<Track[]>(EMPTY_TRACKS);
  const [featuredTracks, setFeaturedTracks] = useState<Track[]>(EMPTY_TRACKS);
  const [playbackState, setPlaybackState] = useState<PlaybackState>(DEFAULT_PLAYBACK_STATE);
  const [queueState, setQueueState] = useState<QueueState>(DEFAULT_QUEUE_STATE);
  const [audioOutputDevices, setAudioOutputDevices] = useState<AudioOutputDevice[]>([]);
  const [equalizerState, setEqualizerState] = useState<EqualizerState>(DEFAULT_EQUALIZER_STATE);
  const [isTransportBusy, setIsTransportBusy] = useState(false);
  const [statusOverride, setStatusOverride] = useState<PlaybackStatusView | null>(null);

  const pollingTimerRef = useRef<number | null>(null);
  const featuredLoadTimerRef = useRef<number | null>(null);
  const hasInitializedRef = useRef(false);
  const previousPlaybackStateRef = useRef<PlaybackState | null>(null);
  const isTransportBusyRef = useRef(false);
  const isAutoAdvancingRef = useRef(false);
  const isStartingPlaybackRef = useRef(false);
  const suppressPlaybackErrorsUntilRef = useRef(0);

  const setTransportBusy = (busy: boolean) => {
    isTransportBusyRef.current = busy;
    setIsTransportBusy(busy);
  };

  const applyQueueSnapshot = (nextQueueState: QueueState) => {
    setQueueState(nextQueueState);
  };

  const applyEqualizerSnapshot = (nextEqualizerState: EqualizerState) => {
    setEqualizerState(nextEqualizerState);
  };

  const setPlaybackOverride = (
    label: string,
    details: string,
    visualState: PlaybackStatus = "error",
  ) => {
    setStatusOverride({ label, details, visualState });
  };

  const runTransportCommand = async (command: () => Promise<void>): Promise<boolean> => {
    if (isTransportBusyRef.current) {
      return false;
    }

    setTransportBusy(true);
    try {
      await command();
      return true;
    } finally {
      setTransportBusy(false);
    }
  };

  const refreshQueueState = useEffectEvent(async () => {
    const response = await nativeBridge.getQueueState();
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось получить состояние очереди.");
    }

    applyQueueSnapshot({
      currentTrackId: response.currentTrackId,
      currentTrackTitle: response.currentTrackTitle,
      queuedTracks: response.queuedTracks,
      shuffleEnabled: response.shuffleEnabled,
      repeatMode: response.repeatMode,
      canPlayPrevious: response.canPlayPrevious,
      canPlayNext: response.canPlayNext,
    });
  });

  const refreshAudioOutputDevices = useEffectEvent(async () => {
    const response = await nativeBridge.getAudioOutputDevices();
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось получить список устройств вывода.");
    }

    setAudioOutputDevices(response.devices);
  });

  const playCompletionFollowUpIfAvailable = useEffectEvent(async () => {
    if (isAutoAdvancingRef.current) {
      return;
    }

    isAutoAdvancingRef.current = true;

    try {
      const commandExecuted = await runTransportCommand(async () => {
        const response = await nativeBridge.playCompletionFollowUp();
        if (response.ok === false) {
          throw new Error(
            response.message || "Не удалось продолжить playback после окончания трека.",
          );
        }

        if (response.advanced === false) {
          return;
        }

        isStartingPlaybackRef.current = true;
        suppressPlaybackErrorsUntilRef.current = Date.now() + 2500;
        setStatusOverride({
          label: "Загрузка",
          details: response.trackTitle || "Подготавливаем трек...",
          visualState: "loading",
        });

        await refreshQueueState();
      });

      if (!commandExecuted) {
        return;
      }

      await refreshPlaybackState();
    } finally {
      isAutoAdvancingRef.current = false;
    }
  });

  const refreshPlaybackState = useEffectEvent(async () => {
    const response = await nativeBridge.getPlaybackState();
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось получить playback state.");
    }

    const nextPlaybackState: PlaybackState = {
      state: response.state,
      streamUrl: response.streamUrl,
      errorMessage: response.errorMessage,
      positionMs: response.positionMs,
      durationMs: response.durationMs,
      volumePercent: response.volumePercent,
      completionToken: response.completionToken,
      trackId: response.trackId,
      trackTitle: response.trackTitle,
      shuffleEnabled: response.shuffleEnabled,
      repeatMode: response.repeatMode,
      canPlayPrevious: response.canPlayPrevious,
      canPlayNext: response.canPlayNext,
    };

    previousPlaybackStateRef.current = playbackState;
    setPlaybackState(nextPlaybackState);
    applyQueueSnapshot({
      currentTrackId: response.trackId || queueState.currentTrackId,
      currentTrackTitle: response.trackTitle || queueState.currentTrackTitle,
      queuedTracks: queueState.queuedTracks,
      shuffleEnabled: response.shuffleEnabled,
      repeatMode: response.repeatMode,
      canPlayPrevious: response.canPlayPrevious,
      canPlayNext: response.canPlayNext,
    });

    const isPlaybackActiveState = ["loading", "playing", "paused"].includes(nextPlaybackState.state);
    if (isPlaybackActiveState && nextPlaybackState.trackId) {
      isStartingPlaybackRef.current = false;
      suppressPlaybackErrorsUntilRef.current = 0;
      setStatusOverride(null);
    }

    if (nextPlaybackState.state === "error") {
      if (isStartingPlaybackRef.current && Date.now() < suppressPlaybackErrorsUntilRef.current) {
        return;
      }

      isStartingPlaybackRef.current = false;
      setPlaybackOverride(
        mapPlaybackStateLabel(nextPlaybackState.state),
        response.errorMessage || "Неизвестная ошибка.",
      );
      return;
    }

    if (hasPlaybackEnded(previousPlaybackStateRef.current, nextPlaybackState)) {
      await playCompletionFollowUpIfAvailable();
    }
  });

  const refreshEqualizerState = useEffectEvent(async () => {
    const response = await nativeBridge.getEqualizerState();
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось получить состояние эквалайзера.");
    }

    applyEqualizerSnapshot(response);
  });

  const initializeBridge = useEffectEvent(async () => {
    try {
      const appInfo = await nativeBridge.getAppInfo();
      if (appInfo.ok === false) {
        throw new Error(appInfo.message || "Не удалось получить app info.");
      }
      setBridgeTitle(appInfo.bridgeStatus);
      setBridgeDetails(appInfo.applicationName);
      setPlaybackBackend(`Playback backend: ${appInfo.playbackBackend || "неизвестен"}`);
      return true;
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      setBridgeTitle("Ошибка bridge");
      setBridgeDetails(errorMessage);
      return false;
    }
  });

  useEffect(() => {
    document.body.dataset.playbackState = statusOverride?.visualState || playbackState.state || "idle";
    document.title = playbackState.trackTitle ? `${playbackState.trackTitle} - LilMusic` : "LilMusic";
  }, [playbackState.state, playbackState.trackTitle, statusOverride]);

  useEffect(() => {
    if (hasInitializedRef.current) {
      return;
    }

    hasInitializedRef.current = true;

    const initialize = async () => {
      const bridgeAvailable = await initializeBridge();
      if (!bridgeAvailable) {
        return;
      }

      try {
        await refreshQueueState();
        await refreshAudioOutputDevices();
        await refreshPlaybackState();
        await refreshEqualizerState();
      } catch (error) {
        const errorMessage = error instanceof Error ? error.message : String(error);
        setPlaybackOverride("Ошибка инициализации", errorMessage);
      }

      featuredLoadTimerRef.current = window.setTimeout(() => {
        void loadFeaturedTracks();
      }, 50);

      if (pollingTimerRef.current !== null) {
        window.clearInterval(pollingTimerRef.current);
      }

      pollingTimerRef.current = window.setInterval(() => {
        void refreshPlaybackState().catch((error) => {
          const errorMessage = error instanceof Error ? error.message : String(error);
          setPlaybackOverride("Ошибка playback", errorMessage);
        });
      }, 1000);
    };

    void initialize();

    return () => {
      if (pollingTimerRef.current !== null) {
        window.clearInterval(pollingTimerRef.current);
        pollingTimerRef.current = null;
      }

      if (featuredLoadTimerRef.current !== null) {
        window.clearTimeout(featuredLoadTimerRef.current);
        featuredLoadTimerRef.current = null;
      }
    };
  }, []);

  const setTab = (tabId: TabId) => {
    startTransition(() => {
      setActiveTabId(tabId);
    });
  };

  const loadFeaturedTracks = useEffectEvent(async () => {
    if (isOfflineMode()) {
      setFeaturedSummary("Нет подключения к сети. Главная лента появится, когда интернет вернётся.");
      startTransition(() => {
        setFeaturedTracks([]);
      });
      return;
    }

    setFeaturedSummary("Загружаем популярные треки...");

    try {
      const response = await nativeBridge.getFeaturedTracks(12);
      if (response.ok === false) {
        throw new Error(response.message || "Не удалось загрузить популярные треки.");
      }

      startTransition(() => {
        setFeaturedTracks(response.tracks);
      });
      setFeaturedSummary(
        `${response.title || "Популярное сейчас"}: ${response.tracks.length} совместимых трек(ов).`,
      );
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      setFeaturedSummary(`Ошибка загрузки стартовой витрины: ${errorMessage}`);
      startTransition(() => {
        setFeaturedTracks([]);
      });
    }
  });

  const submitSearch = async () => {
    if (isOfflineMode()) {
      setSearchSummary("Нет подключения к сети. Поиск по SoundCloud временно недоступен.");
      startTransition(() => {
        setSearchTracks([]);
        setActiveTabId("search");
      });
      return;
    }

    setSearchSummary("Отправляем запрос в native bridge...");
    startTransition(() => {
      setActiveTabId("search");
    });

    try {
      const response = await nativeBridge.searchTracks(searchQuery.trim(), searchLimit, searchOffset);
      if (response.ok === false) {
        throw new Error(response.message || "Поиск через bridge завершился ошибкой.");
      }

      startTransition(() => {
        setSearchTracks(response.tracks);
      });
      setSearchSummary(
        `Результаты по "${response.query}": ${response.tracks.length} совместимых трек(ов), limit=${response.limit}, offset=${response.offset}.`,
      );
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      setSearchSummary(`Ошибка запроса к bridge: ${errorMessage}`);
      startTransition(() => {
        setSearchTracks([]);
      });
    }
  };

  const playTrack = async (track: Track) => {
    if (isOfflineMode()) {
      setPlaybackOverride("Нет сети", "Для запуска нового трека требуется доступ к SoundCloud API.");
      return;
    }

    await runTransportCommand(async () => {
      isStartingPlaybackRef.current = true;
      suppressPlaybackErrorsUntilRef.current = Date.now() + 2500;
      setStatusOverride({
        label: "Подготовка...",
        details: `Запрашиваем native playback для: ${track.title}`,
        visualState: "loading",
      });

      const response = await nativeBridge.playTrack(track.id, track.title);
      if (response.ok === false) {
        isStartingPlaybackRef.current = false;
        suppressPlaybackErrorsUntilRef.current = 0;
        throw new Error(response.message || "Не удалось запустить воспроизведение.");
      }

      setPlaybackState((currentPlaybackState) => ({
        ...currentPlaybackState,
        state: "loading",
        trackId: response.trackId || track.id,
        trackTitle: response.trackTitle || track.title,
        positionMs: 0,
        durationMs: 0,
      }));
      setStatusOverride({
        label: "Загрузка",
        details: response.trackTitle || track.title,
        visualState: "loading",
      });

      await refreshQueueState();
    });

    await refreshPlaybackState();
  };

  const enqueueTrack = async (track: Track) => {
    const response = await nativeBridge.enqueueTrack(track.id);
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось добавить трек в очередь.");
    }

    await refreshQueueState();
  };

  const removeQueuedTrack = async (trackId: string) => {
    const response = await nativeBridge.removeQueuedTrack(trackId);
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось удалить трек из очереди.");
    }

    await refreshQueueState();
  };

  const togglePlayback = async () => {
    if (!playbackState.trackId) {
      return;
    }

    const response =
      playbackState.state === "paused"
        ? await nativeBridge.resumePlayback()
        : await nativeBridge.pausePlayback();

    if (response.ok === false) {
      throw new Error(
        response.message ||
          (playbackState.state === "paused"
            ? "Не удалось возобновить воспроизведение."
            : "Не удалось поставить воспроизведение на паузу."),
      );
    }

    await refreshPlaybackState();
  };

  const playPreviousTrack = async () => {
    await runTransportCommand(async () => {
      const response = await nativeBridge.playPreviousTrack();
      if (response.ok === false) {
        throw new Error(response.message || "Предыдущий трек недоступен.");
      }

      isStartingPlaybackRef.current = true;
      suppressPlaybackErrorsUntilRef.current = Date.now() + 2500;
      setPlaybackState((currentPlaybackState) => ({
        ...currentPlaybackState,
        state: "loading",
        trackId: response.trackId || "",
        trackTitle: response.trackTitle || "",
        positionMs: 0,
        durationMs: 0,
      }));
      setStatusOverride({
        label: "Загрузка",
        details: response.trackTitle || "Подготавливаем трек...",
        visualState: "loading",
      });
      await refreshQueueState();
    });

    await refreshPlaybackState();
  };

  const playNextTrack = async () => {
    if (isOfflineMode()) {
      setPlaybackOverride(
        "Нет сети",
        "Следующий трек нельзя подготовить без доступа к SoundCloud API.",
      );
      return;
    }

    await runTransportCommand(async () => {
      const response = await nativeBridge.playNextTrack();
      if (response.ok === false) {
        throw new Error(response.message || "Следующий трек недоступен.");
      }

      isStartingPlaybackRef.current = true;
      suppressPlaybackErrorsUntilRef.current = Date.now() + 2500;
      setPlaybackState((currentPlaybackState) => ({
        ...currentPlaybackState,
        state: "loading",
        trackId: response.trackId || "",
        trackTitle: response.trackTitle || "",
        positionMs: 0,
        durationMs: 0,
      }));
      setStatusOverride({
        label: "Загрузка",
        details: response.trackTitle || "Подготавливаем трек...",
        visualState: "loading",
      });
      await refreshQueueState();
    });

    await refreshPlaybackState();
  };

  const seekPlayback = async (positionMs: number) => {
    const response = await nativeBridge.seekPlayback(positionMs);
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось перемотать трек.");
    }

    setPlaybackState((currentPlaybackState) => ({
      ...currentPlaybackState,
      positionMs: response.positionMs,
    }));
    await refreshPlaybackState();
  };

  const setPlaybackVolume = async (volumePercent: number) => {
    const response = await nativeBridge.setPlaybackVolume(clampVolumePercent(volumePercent));
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось изменить громкость.");
    }

    setPlaybackState((currentPlaybackState) => ({
      ...currentPlaybackState,
      volumePercent: response.volumePercent,
    }));
  };

  const selectAudioOutputDevice = async (deviceId: string) => {
    if (playbackState.trackId) {
      setStatusOverride({
        label: "Переключаем вывод",
        details: "Подготавливаем audio path для выбранного устройства.",
        visualState: "loading",
      });
    }

    const response = await nativeBridge.selectAudioOutputDevice(deviceId);
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось переключить устройство вывода.");
    }

    setAudioOutputDevices(response.devices);
    await refreshPlaybackState();
  };

  const toggleShuffle = async () => {
    const response = await nativeBridge.toggleShuffle();
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось переключить shuffle.");
    }

    applyQueueSnapshot({
      currentTrackId: response.currentTrackId,
      currentTrackTitle: response.currentTrackTitle,
      queuedTracks: response.queuedTracks,
      shuffleEnabled: response.shuffleEnabled,
      repeatMode: response.repeatMode,
      canPlayPrevious: response.canPlayPrevious,
      canPlayNext: response.canPlayNext,
    });
  };

  const cycleRepeatMode = async () => {
    const response = await nativeBridge.cycleRepeatMode();
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось переключить repeat.");
    }

    applyQueueSnapshot({
      currentTrackId: response.currentTrackId,
      currentTrackTitle: response.currentTrackTitle,
      queuedTracks: response.queuedTracks,
      shuffleEnabled: response.shuffleEnabled,
      repeatMode: response.repeatMode,
      canPlayPrevious: response.canPlayPrevious,
      canPlayNext: response.canPlayNext,
    });
  };

  const setEqualizerEnabled = async (enabled: boolean) => {
    const response = await nativeBridge.setEqualizerEnabled(enabled);
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось изменить состояние эквалайзера.");
    }

    applyEqualizerSnapshot(response);
  };

  const selectEqualizerPreset = async (presetId: EqualizerPresetId) => {
    const response = await nativeBridge.selectEqualizerPreset(presetId);
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось применить пресет эквалайзера.");
    }

    applyEqualizerSnapshot(response);
  };

  const setEqualizerBandGain = async (bandIndex: number, gainDb: number) => {
    const response = await nativeBridge.setEqualizerBandGain(bandIndex, gainDb);
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось изменить полосу эквалайзера.");
    }

    applyEqualizerSnapshot(response);
  };

  const setEqualizerOutputGain = async (outputGainDb: number) => {
    const response = await nativeBridge.setEqualizerOutputGain(outputGainDb);
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось изменить общий уровень эквалайзера.");
    }

    applyEqualizerSnapshot(response);
  };

  const resetEqualizer = async () => {
    const response = await nativeBridge.resetEqualizer();
    if (response.ok === false) {
      throw new Error(response.message || "Не удалось сбросить эквалайзер.");
    }

    applyEqualizerSnapshot(response);
  };

  const currentTrackId = playbackState.trackId || queueState.currentTrackId;
  const currentTrackTitle =
    playbackState.trackTitle || queueState.currentTrackTitle || "Трек ещё не выбран";
  const playbackStatusView: PlaybackStatusView =
    statusOverride ||
    (playbackState.state === "error"
      ? {
          label: mapPlaybackStateLabel(playbackState.state),
          details: playbackState.errorMessage || "Неизвестная ошибка.",
          visualState: "error",
        }
      : {
          label: mapPlaybackStateLabel(playbackState.state),
          details: currentTrackTitle,
          visualState: playbackState.state,
        });

  const equalizerStatusSummary =
    equalizerState.status === "ready"
      ? equalizerState.enabled
        ? "Эквалайзер включён: полосы меняют частоты, а Level регулирует общий уровень обработанного сигнала."
        : "Эквалайзер выключен, но все полосы и Level сохранены и готовы к повторному включению."
      : equalizerState.errorMessage ||
        {
          loading: "Эквалайзер инициализируется...",
          unsupported_audio_path: "Текущий audio path не поддерживает DSP-цепочку эквалайзера.",
          audio_engine_unavailable:
            "Native audio engine временно недоступен. Попробуй повторить после восстановления устройства.",
          error: "Не удалось применить состояние эквалайзера.",
          ready: "",
        }[equalizerState.status];

  return {
    bridgeTitle,
    bridgeDetails,
    playbackBackend,
    activeTabId,
    searchQuery,
    searchLimit,
    searchOffset,
    searchSummary,
    featuredSummary,
    searchTracks,
    featuredTracks,
    playbackState,
    queueState,
    audioOutputDevices,
    equalizerState,
    isTransportBusy,
    currentTrackId,
    currentTrackTitle,
    playbackStatusView,
    reportStatusError: setPlaybackOverride,
    equalizerStatusSummary,
    equalizerStatusLabel: mapEqualizerStatusLabel(equalizerState.status),
    equalizerPresetTitle: getEqualizerPresetTitle(
      equalizerState.presets,
      equalizerState.activePresetId,
    ),
    setTab,
    setSearchQuery,
    setSearchLimit,
    setSearchOffset,
    submitSearch,
    loadFeaturedTracks,
    playTrack,
    enqueueTrack,
    removeQueuedTrack,
    togglePlayback,
    playPreviousTrack,
    playNextTrack,
    seekPlayback,
    setPlaybackVolume,
    refreshAudioOutputDevices,
    selectAudioOutputDevice,
    toggleShuffle,
    cycleRepeatMode,
    setEqualizerEnabled,
    selectEqualizerPreset,
    setEqualizerBandGain,
    setEqualizerOutputGain,
    resetEqualizer,
  };
}
