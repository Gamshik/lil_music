const bridgeStatusElement = document.getElementById("bridge-status");
const appNameElement = document.getElementById("app-name");
const searchFormElement = document.getElementById("search-form");
const searchQueryElement = document.getElementById("search-query");
const searchLimitElement = document.getElementById("search-limit");
const searchOffsetElement = document.getElementById("search-offset");
const searchSummaryElement = document.getElementById("search-summary");
const featuredSummaryElement = document.getElementById("featured-summary");
const tracksListElement = document.getElementById("tracks-list");
const featuredListElement = document.getElementById("featured-list");
const playbackBackendElement = document.getElementById("playback-backend");
const playbackStatusElement = document.getElementById("playback-status");
const currentTrackElement = document.getElementById("current-track");
const playbackProgressTrackElement = document.getElementById("playback-progress-track");
const playbackProgressFillElement = document.getElementById("playback-progress-fill");
const playbackProgressThumbElement = document.getElementById("playback-progress-thumb");
const playbackPositionElement = document.getElementById("playback-position");
const playbackDurationElement = document.getElementById("playback-duration");
const shuffleButtonElement = document.getElementById("shuffle-button");
const repeatButtonElement = document.getElementById("repeat-button");
const repeatBadgeElement = document.getElementById("repeat-badge");
const prevTrackButtonElement = document.getElementById("prev-track-button");
const playbackToggleButtonElement = document.getElementById("playback-toggle-button");
const nextTrackButtonElement = document.getElementById("next-track-button");
const volumeSliderElement = document.getElementById("volume-slider");
const volumeValueElement = document.getElementById("volume-value");
const loadFeaturedButtonElement = document.getElementById("load-featured-button");
const queueCountElement = document.getElementById("queue-count");
const queueListElement = document.getElementById("queue-list");
const equalizerSummaryElement = document.getElementById("equalizer-summary");
const equalizerStatusPillElement = document.getElementById("equalizer-status-pill");
const equalizerStatusDetailElement = document.getElementById("equalizer-status-detail");
const equalizerActivePresetElement = document.getElementById("equalizer-active-preset");
const equalizerHeadroomElement = document.getElementById("equalizer-headroom");
const equalizerToggleButtonElement = document.getElementById("equalizer-toggle-button");
const equalizerResetButtonElement = document.getElementById("equalizer-reset-button");
const equalizerPresetsElement = document.getElementById("equalizer-presets");
const equalizerBandsElement = document.getElementById("equalizer-bands");
const tabButtonElements = Array.from(document.querySelectorAll("[data-tab-target]"));
const tabPanelElements = Array.from(document.querySelectorAll("[data-tab-panel]"));
const pageRootElement = document.body;

let currentTrackTitle = "";
let currentTrackId = "";
let hasLoadedTrack = false;
let isPlaybackPaused = false;
let playbackPollingTimer = null;
let searchRenderedTracks = [];
let featuredRenderedTracks = [];
let latestPlaybackState = {
  state: "idle",
  positionMs: 0,
  durationMs: 0,
  volumePercent: 100,
  completionToken: 0,
  trackId: "",
  trackTitle: "",
};
let latestQueueState = {
  currentTrackId: "",
  currentTrackTitle: "",
  queuedTracks: [],
  shuffleEnabled: false,
  repeatMode: "none",
  canPlayPrevious: false,
  canPlayNext: false,
};
let latestEqualizerState = {
  status: "loading",
  enabled: false,
  activePresetId: "flat",
  outputGainDb: 0,
  headroomCompensationDb: 0,
  errorMessage: "",
  bands: [],
  presets: [],
};
let previousPlaybackState = null;
let isAutoAdvancing = false;
let isStartingPlayback = false;
let isTransportCommandInFlight = false;
let suppressPlaybackErrorsUntilMs = 0;
let activeTabId = "home";
let isProgressDragging = false;
let progressDragRatio = 0;
let progressDragPointerId = null;
let volumeSyncTimer = null;
let outputGainSyncTimer = null;
const equalizerBandSyncTimers = new Map();
let equalizerDragPointerId = null;
let equalizerDragElement = null;

function normalizeBridgePayload(payload) {
  // Bridge может вернуть как уже распарсенный объект, так и JSON-строку.
  // Нормализуем оба варианта в единый формат для остального UI-кода.
  if (typeof payload !== "string") {
    return payload;
  }

  try {
    return JSON.parse(payload);
  } catch {
    return payload;
  }
}

function setBridgeStatus(title, details) {
  bridgeStatusElement.textContent = title;
  appNameElement.textContent = details;
}

function setActiveTab(nextTabId) {
  // UI deliberately держит вкладки локально на web-слое:
  // это presentation-state, который не нужен native-слою.
  activeTabId = nextTabId;

  tabButtonElements.forEach((tabButtonElement) => {
    const isActive = tabButtonElement.dataset.tabTarget === nextTabId;
    tabButtonElement.classList.toggle("is-active", isActive);
    tabButtonElement.setAttribute("aria-selected", isActive ? "true" : "false");
  });

  tabPanelElements.forEach((tabPanelElement) => {
    const isActive = tabPanelElement.dataset.tabPanel === nextTabId;
    tabPanelElement.classList.toggle("is-active", isActive);
    tabPanelElement.hidden = !isActive;
  });
}

function setPlaybackVisualState(state) {
  // Состояние playback прокидываем в data-атрибут body, чтобы CSS мог
  // анимировать shell и visualizer без ручного управления классами в каждом месте.
  if (pageRootElement) {
    pageRootElement.dataset.playbackState = state || "idle";
  }

  if (typeof document !== "undefined") {
    document.title = currentTrackTitle ? `${currentTrackTitle} - LilMusic` : "LilMusic";
  }
}

function isOfflineMode() {
  return typeof navigator !== "undefined" && navigator.onLine === false;
}

function setPlaybackStatus(state, details) {
  playbackStatusElement.textContent = state;
  currentTrackElement.textContent = details;
}

function formatMilliseconds(milliseconds) {
  const totalSeconds = Math.max(0, Math.floor(milliseconds / 1000));
  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;
  return `${minutes}:${String(seconds).padStart(2, "0")}`;
}

function setPlaybackProgress(positionMs, durationMs) {
  const safePosition = Math.max(0, positionMs || 0);
  const safeDuration = Math.max(0, durationMs || 0);
  const progressPercent = safeDuration > 0 ? Math.min(100, (safePosition / safeDuration) * 100) : 0;

  playbackProgressFillElement.style.width = `${progressPercent}%`;
  playbackProgressThumbElement.style.left = `${progressPercent}%`;
  playbackProgressTrackElement.setAttribute("aria-valuenow", String(Math.round(progressPercent)));
  playbackPositionElement.textContent = formatMilliseconds(safePosition);
  playbackDurationElement.textContent = formatMilliseconds(safeDuration);
}

function setVolumeLevel(volumePercent) {
  const safeVolumePercent = Math.max(0, Math.min(100, Number(volumePercent) || 0));
  volumeSliderElement.value = String(safeVolumePercent);
  volumeValueElement.textContent = `${safeVolumePercent}%`;
}

function setPlaybackToggleState({ disabled, paused }) {
  playbackToggleButtonElement.disabled = disabled;
  playbackToggleButtonElement.textContent = paused ? "Продолжить" : "Пауза";
}

function setShuffleState(shuffleEnabled) {
  shuffleButtonElement.classList.toggle("is-active", Boolean(shuffleEnabled));
  shuffleButtonElement.setAttribute("aria-pressed", shuffleEnabled ? "true" : "false");
}

function setRepeatMode(repeatMode) {
  const safeRepeatMode =
    repeatMode === "playlist" || repeatMode === "track" ? repeatMode : "none";
  repeatButtonElement.dataset.repeatMode = safeRepeatMode;
  repeatButtonElement.setAttribute("aria-pressed", safeRepeatMode === "none" ? "false" : "true");

  if (safeRepeatMode === "playlist") {
    repeatButtonElement.title = "Повтор списка";
    repeatBadgeElement.textContent = "•";
  } else if (safeRepeatMode === "track") {
    repeatButtonElement.title = "Повтор трека";
    repeatBadgeElement.textContent = "1";
  } else {
    repeatButtonElement.title = "Повтор выключен";
    repeatBadgeElement.textContent = "";
  }
}

function mapEqualizerStatusLabel(status) {
  switch (status) {
    case "ready":
      return "EQ готов";
    case "unsupported_audio_path":
      return "Неподдерживаемый audio path";
    case "audio_engine_unavailable":
      return "Audio engine недоступен";
    case "error":
      return "Ошибка EQ";
    default:
      return "Загрузка EQ...";
  }
}

function getEqualizerPresetTitle(presetId) {
  const matchingPreset = latestEqualizerState.presets.find((preset) => preset.id === presetId);
  return matchingPreset?.title || "Flat";
}

function formatEqualizerGainDb(gainDb) {
  const safeGainDb = Number(gainDb) || 0;
  return `${safeGainDb > 0 ? "+" : ""}${safeGainDb.toFixed(1)} dB`;
}

function formatEqualizerFrequencyLabel(centerFrequencyHz) {
  const safeFrequencyHz = Number(centerFrequencyHz) || 0;
  if (safeFrequencyHz >= 1000) {
    const valueInKhz = safeFrequencyHz / 1000;
    return `${Number.isInteger(valueInKhz) ? valueInKhz.toFixed(0) : valueInKhz.toFixed(1)}k`;
  }

  return `${Math.round(safeFrequencyHz)}`;
}

function clampEqualizerGainDb(gainDb) {
  return Math.max(-12, Math.min(12, Math.round((Number(gainDb) || 0) * 2) / 2));
}

function deriveEqualizerPresetIdFromCurrentBands() {
  const matchingPreset = latestEqualizerState.presets.find((preset) => {
    if (preset.id === "custom" || !Array.isArray(preset.gainsDb)) {
      return false;
    }

    return preset.gainsDb.every((gainDb, index) => {
      const band = latestEqualizerState.bands[index];
      return band && Math.abs((Number(band.gainDb) || 0) - (Number(gainDb) || 0)) < 0.01;
    });
  });

  return matchingPreset?.id || "custom";
}

function setEqualizerStatusSummary() {
  const statusLabel = mapEqualizerStatusLabel(latestEqualizerState.status);
  equalizerStatusPillElement.textContent = statusLabel;
  equalizerStatusDetailElement.textContent =
    latestEqualizerState.errorMessage ||
    (latestEqualizerState.status === "ready"
      ? "DSP активен в native audio engine."
      : "Состояние EQ синхронизируется с native backend.");
  equalizerActivePresetElement.textContent = getEqualizerPresetTitle(
    latestEqualizerState.activePresetId,
  );
  equalizerHeadroomElement.textContent = `Level: ${formatEqualizerGainDb(
    latestEqualizerState.outputGainDb,
  )} · Auto headroom: ${formatEqualizerGainDb(latestEqualizerState.headroomCompensationDb)}`;

  if (latestEqualizerState.status === "ready") {
    equalizerSummaryElement.textContent = latestEqualizerState.enabled
      ? "Эквалайзер включён: полосы меняют частоты, а Level регулирует общий уровень обработанного сигнала."
      : "Эквалайзер выключен, но все полосы и Level сохранены и готовы к повторному включению.";
  } else if (latestEqualizerState.status === "unsupported_audio_path") {
    equalizerSummaryElement.textContent =
      latestEqualizerState.errorMessage ||
      "Текущий audio path не поддерживает DSP-цепочку эквалайзера.";
  } else if (latestEqualizerState.status === "audio_engine_unavailable") {
    equalizerSummaryElement.textContent =
      latestEqualizerState.errorMessage ||
      "Native audio engine временно недоступен. Попробуй повторить после восстановления устройства.";
  } else if (latestEqualizerState.status === "error") {
    equalizerSummaryElement.textContent =
      latestEqualizerState.errorMessage || "Не удалось применить состояние эквалайзера.";
  } else {
    equalizerSummaryElement.textContent = "Эквалайзер инициализируется...";
  }
}

function applyEqualizerSliderVisualState(sliderElement, gainDb) {
  const safeGainDb = clampEqualizerGainDb(gainDb);
  const normalized = (12 - safeGainDb) / 24;
  const thumbTopPercent = Math.max(0, Math.min(100, normalized * 100));
  const fillElement = sliderElement.querySelector(".equalizer-slider-fill");
  const thumbElement = sliderElement.querySelector(".equalizer-slider-thumb");
  const valueElement = sliderElement.closest(".equalizer-band")?.querySelector(".equalizer-band-value");
  const sliderLabel = sliderElement.dataset.sliderLabel || "Эквалайзер";

  sliderElement.dataset.valueDb = String(safeGainDb);
  sliderElement.setAttribute("aria-label", `${sliderLabel}, ${formatEqualizerGainDb(safeGainDb)}`);
  sliderElement.setAttribute("aria-valuenow", safeGainDb.toFixed(1));
  sliderElement.setAttribute("aria-valuetext", formatEqualizerGainDb(safeGainDb));

  if (valueElement) {
    valueElement.textContent = formatEqualizerGainDb(safeGainDb);
  }

  if (thumbElement) {
    thumbElement.style.top = `${thumbTopPercent}%`;
  }

  if (fillElement) {
    if (safeGainDb >= 0) {
      fillElement.style.top = `${thumbTopPercent}%`;
      fillElement.style.height = `${50 - thumbTopPercent}%`;
    } else {
      fillElement.style.top = "50%";
      fillElement.style.height = `${thumbTopPercent - 50}%`;
    }
  }
}

function syncEqualizerPresetButtons() {
  const presetButtonElements = equalizerPresetsElement.querySelectorAll(".equalizer-preset-button");
  presetButtonElements.forEach((presetButtonElement) => {
    const isActive = presetButtonElement.dataset.presetId === latestEqualizerState.activePresetId;
    presetButtonElement.classList.toggle("is-active", isActive);
    presetButtonElement.setAttribute("aria-pressed", isActive ? "true" : "false");
  });
}

function createEqualizerSliderElement({ kind, label, valueDb, bandIndex, controlsEnabled }) {
  const bandElement = document.createElement("div");
  bandElement.className = "equalizer-band";

  const valueElement = document.createElement("span");
  valueElement.className = "equalizer-band-value";

  const sliderElement = document.createElement("div");
  sliderElement.className = `equalizer-slider${kind === "output" ? " is-output" : ""}${
    controlsEnabled ? "" : " is-disabled"
  }`;
  sliderElement.dataset.action = "equalizer-slider";
  sliderElement.dataset.sliderKind = kind;
  if (typeof bandIndex === "number") {
    sliderElement.dataset.bandIndex = String(bandIndex);
  }
  sliderElement.dataset.sliderLabel = label;
  sliderElement.tabIndex = controlsEnabled ? 0 : -1;
  sliderElement.setAttribute("role", "slider");
  sliderElement.setAttribute("aria-valuemin", "-12");
  sliderElement.setAttribute("aria-valuemax", "12");

  const trackElement = document.createElement("div");
  trackElement.className = "equalizer-slider-track";

  const railElement = document.createElement("div");
  railElement.className = "equalizer-slider-rail";

  const zeroLineElement = document.createElement("div");
  zeroLineElement.className = "equalizer-slider-zero-line";

  const fillElement = document.createElement("div");
  fillElement.className = "equalizer-slider-fill";

  const thumbElement = document.createElement("div");
  thumbElement.className = "equalizer-slider-thumb";
  thumbElement.setAttribute("aria-hidden", "true");

  const labelElement = document.createElement("span");
  labelElement.className = "equalizer-band-frequency";
  labelElement.textContent = label;

  trackElement.append(railElement, zeroLineElement, fillElement, thumbElement);
  bandElement.append(valueElement, sliderElement, labelElement);
  sliderElement.append(trackElement);
  applyEqualizerSliderVisualState(sliderElement, valueDb);
  return bandElement;
}

function renderEqualizer() {
  const controlsEnabled = latestEqualizerState.status === "ready";
  equalizerToggleButtonElement.disabled = !controlsEnabled;
  equalizerToggleButtonElement.setAttribute(
    "aria-pressed",
    latestEqualizerState.enabled ? "true" : "false",
  );
  equalizerResetButtonElement.disabled = !controlsEnabled;
  setEqualizerStatusSummary();

  equalizerPresetsElement.replaceChildren();
  latestEqualizerState.presets.forEach((preset) => {
    const presetButtonElement = document.createElement("button");
    presetButtonElement.className = "equalizer-preset-button";
    presetButtonElement.type = "button";
    presetButtonElement.dataset.action = "select-equalizer-preset";
    presetButtonElement.dataset.presetId = preset.id;
    presetButtonElement.textContent = preset.title;
    presetButtonElement.disabled = !controlsEnabled || preset.id === "custom";
    presetButtonElement.classList.toggle(
      "is-active",
      preset.id === latestEqualizerState.activePresetId,
    );
    presetButtonElement.classList.toggle("is-derived", preset.id === "custom");
    presetButtonElement.setAttribute(
      "aria-pressed",
      preset.id === latestEqualizerState.activePresetId ? "true" : "false",
    );
    equalizerPresetsElement.append(presetButtonElement);
  });

  equalizerBandsElement.replaceChildren();
  equalizerBandsElement.append(
    createEqualizerSliderElement({
      kind: "output",
      label: "level",
      valueDb: latestEqualizerState.outputGainDb,
      controlsEnabled,
    }),
  );

  latestEqualizerState.bands.forEach((band) => {
    equalizerBandsElement.append(
      createEqualizerSliderElement({
        kind: "band",
        label: formatEqualizerFrequencyLabel(band.centerFrequencyHz),
        valueDb: band.gainDb,
        bandIndex: band.index,
        controlsEnabled,
      }),
    );
  });

  syncEqualizerPresetButtons();
}

function applyEqualizerStateSnapshot(snapshot) {
  latestEqualizerState = {
    status: snapshot.status || latestEqualizerState.status || "loading",
    enabled: Boolean(snapshot.enabled),
    activePresetId: snapshot.activePresetId || latestEqualizerState.activePresetId || "flat",
    outputGainDb:
      typeof snapshot.outputGainDb === "number"
        ? snapshot.outputGainDb
        : latestEqualizerState.outputGainDb || 0,
    headroomCompensationDb:
      typeof snapshot.headroomCompensationDb === "number"
        ? snapshot.headroomCompensationDb
        : latestEqualizerState.headroomCompensationDb || 0,
    errorMessage: snapshot.errorMessage || "",
    bands: Array.isArray(snapshot.bands) ? snapshot.bands : latestEqualizerState.bands || [],
    presets: Array.isArray(snapshot.presets) ? snapshot.presets : latestEqualizerState.presets || [],
  };

  if (equalizerDragPointerId !== null) {
    setEqualizerStatusSummary();
    syncEqualizerPresetButtons();
    return;
  }

  renderEqualizer();
}

function canSeekPlayback() {
  return (
    hasLoadedTrack &&
    latestPlaybackState.durationMs > 0 &&
    typeof window.seekPlayback === "function"
  );
}

function getProgressRatioFromClientX(clientX) {
  const trackRect = playbackProgressTrackElement.getBoundingClientRect();
  if (trackRect.width <= 0) {
    return 0;
  }

  return Math.min(1, Math.max(0, (clientX - trackRect.left) / trackRect.width));
}

function previewDraggedProgress(ratio) {
  progressDragRatio = Math.min(1, Math.max(0, ratio));
  const previewPositionMs = Math.round(latestPlaybackState.durationMs * progressDragRatio);
  playbackProgressTrackElement.classList.add("is-dragging");
  setPlaybackProgress(previewPositionMs, latestPlaybackState.durationMs);
}

function markPendingTrackSwitch(trackId, trackTitle) {
  // Локально переводим UI в loading сразу после успешной transport-команды,
  // не дожидаясь следующего polling-цикла getPlaybackState().
  latestPlaybackState = {
    ...latestPlaybackState,
    state: "loading",
    trackId: trackId || latestPlaybackState.trackId,
    trackTitle: trackTitle || latestPlaybackState.trackTitle,
    positionMs: 0,
    durationMs: 0,
  };
  currentTrackId = latestPlaybackState.trackId || currentTrackId;
  currentTrackTitle = latestPlaybackState.trackTitle || currentTrackTitle;
  hasLoadedTrack = true;
  isPlaybackPaused = false;
  setPlaybackToggleState({ disabled: true, paused: false });
  setPlaybackProgress(0, 0);
  setPlaybackStatus("Загрузка", currentTrackTitle || "Подготавливаем трек...");
  setPlaybackVisualState("loading");
  updateCurrentTrackHighlights();
  syncTransportButtons();
}

function mapPlaybackStateLabel(state) {
  switch (state) {
    case "loading":
      return "Загрузка";
    case "playing":
      return "Воспроизведение";
    case "paused":
      return "Пауза";
    case "error":
      return "Ошибка playback";
    default:
      return "Остановлено";
  }
}

function createArtworkFallbackElement(title) {
  const fallbackElement = document.createElement("div");
  fallbackElement.className = "track-artwork-fallback";
  fallbackElement.textContent = (title || "?").trim().charAt(0).toUpperCase() || "?";
  return fallbackElement;
}

function createArtworkElement(track) {
  const artworkFrameElement = document.createElement("div");
  artworkFrameElement.className = "track-artwork-frame";

  if (!track.artworkUrl) {
    artworkFrameElement.append(createArtworkFallbackElement(track.title));
    return artworkFrameElement;
  }

  const artworkImageElement = document.createElement("img");
  artworkImageElement.className = "track-artwork";
  artworkImageElement.src = track.artworkUrl;
  artworkImageElement.alt = `Обложка ${track.title}`;
  artworkImageElement.loading = "lazy";
  artworkImageElement.referrerPolicy = "no-referrer";
  artworkImageElement.addEventListener("error", () => {
    artworkFrameElement.replaceChildren(createArtworkFallbackElement(track.title));
  });

  artworkFrameElement.append(artworkImageElement);
  return artworkFrameElement;
}

function findTrackById(trackId) {
  // Один и тот же трек может быть виден в разных представлениях:
  // в поиске, на главной витрине или в очереди. Для UI-действий
  // сначала собираем его из любого актуального списка.
  return (
    searchRenderedTracks.find((track) => track.id === trackId) ||
    featuredRenderedTracks.find((track) => track.id === trackId) ||
    latestQueueState.queuedTracks.find((track) => track.id === trackId) ||
    null
  );
}

function updateCurrentTrackHighlights() {
  // Подсветка текущего трека полностью вычисляется на UI-стороне
  // на основе trackId из playback state и queue state.
  const trackRowElements = document.querySelectorAll(".track-row[data-track-id], .queue-item[data-track-id]");
  trackRowElements.forEach((trackRowElement) => {
    trackRowElement.classList.toggle("is-current", trackRowElement.dataset.trackId === currentTrackId);
  });
}

function syncTransportButtons() {
  const shouldDisableTransportButtons =
    isTransportCommandInFlight || latestPlaybackState.state === "loading";
  prevTrackButtonElement.disabled =
    shouldDisableTransportButtons || !latestQueueState.canPlayPrevious;
  nextTrackButtonElement.disabled = shouldDisableTransportButtons || !latestQueueState.canPlayNext;
}

function renderQueue() {
  // Очередь приходит из native session-state и здесь только отображается.
  const queuedTracks = latestQueueState.queuedTracks;
  queueCountElement.textContent =
    queuedTracks.length === 0 ? "Очередь пуста" : `${queuedTracks.length} в очереди`;
  queueListElement.replaceChildren();

  if (queuedTracks.length === 0) {
    const emptyQueueElement = document.createElement("li");
    emptyQueueElement.className = "queue-item queue-item-empty";
    emptyQueueElement.textContent = "Следующим будет трек по текущему списку.";
    queueListElement.append(emptyQueueElement);
    updateCurrentTrackHighlights();
    syncTransportButtons();
    return;
  }

  queuedTracks.forEach((track) => {
    const queueItemElement = document.createElement("li");
    queueItemElement.className = "queue-item";
    queueItemElement.dataset.trackId = track.id;

    const queueMetaElement = document.createElement("div");
    queueMetaElement.className = "queue-item-meta";

    const queueTitleElement = document.createElement("span");
    queueTitleElement.className = "queue-item-title";
    queueTitleElement.textContent = track.title;

    const queueArtistElement = document.createElement("span");
    queueArtistElement.className = "queue-item-artist";
    queueArtistElement.textContent = track.artistName || "Неизвестный артист";

    const removeButtonElement = document.createElement("button");
    removeButtonElement.className = "queue-remove-button";
    removeButtonElement.type = "button";
    removeButtonElement.dataset.action = "remove-queued-track";
    removeButtonElement.dataset.trackId = track.id;
    removeButtonElement.textContent = "Убрать";

    queueMetaElement.append(queueTitleElement, queueArtistElement);
    queueItemElement.append(queueMetaElement, removeButtonElement);
    queueListElement.append(queueItemElement);
  });

  updateCurrentTrackHighlights();
  syncTransportButtons();
}

async function refreshQueueState() {
  if (typeof window.getQueueState !== "function") {
    return;
  }

  // Очередь остаётся source of truth на native-слое, поэтому UI всегда
  // перечитывает её через bridge после enqueue/remove/transport событий.
  const response = normalizeBridgePayload(await window.getQueueState());
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось получить состояние очереди.");
  }

  latestQueueState = {
    currentTrackId: response.currentTrackId || "",
    currentTrackTitle: response.currentTrackTitle || "",
    queuedTracks: Array.isArray(response.queuedTracks) ? response.queuedTracks : [],
    shuffleEnabled: Boolean(response.shuffleEnabled),
    repeatMode: response.repeatMode || "none",
    canPlayPrevious: Boolean(response.canPlayPrevious),
    canPlayNext: Boolean(response.canPlayNext),
  };
  setShuffleState(latestQueueState.shuffleEnabled);
  setRepeatMode(latestQueueState.repeatMode);
  renderQueue();
}

async function enqueueTrackRequest(track) {
  if (!track || !track.id || typeof window.enqueueTrack !== "function") {
    return;
  }

  // После успешного enqueue сразу синхронизируем queue state заново,
  // чтобы не дублировать правила вычисления очереди в JavaScript.
  const response = normalizeBridgePayload(await window.enqueueTrack({ trackId: track.id }));
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось добавить трек в очередь.");
  }

  await refreshQueueState();
}

function renderTracks(tracks, emptyMessage) {
  searchRenderedTracks = Array.isArray(tracks) ? tracks : [];
  renderTrackCollection(tracksListElement, searchRenderedTracks, emptyMessage);
}

function renderFeaturedTracks(tracks, emptyMessage) {
  featuredRenderedTracks = Array.isArray(tracks) ? tracks : [];
  renderTrackCollection(featuredListElement, featuredRenderedTracks, emptyMessage);
}

function renderTrackCollection(listElement, tracks, emptyMessage) {
  // Главная и поиск рендерятся одним и тем же списочным шаблоном,
  // различается только источник данных и контейнер.
  listElement.replaceChildren();

  if (tracks.length === 0) {
    const emptyStateElement = document.createElement("li");
    emptyStateElement.className = "queue-item queue-item-empty";
    emptyStateElement.textContent = emptyMessage;
    listElement.append(emptyStateElement);
    updateCurrentTrackHighlights();
    syncTransportButtons();
    return;
  }

  tracks.forEach((track) => {
    const trackElement = document.createElement("li");
    trackElement.className = "track-row";
    trackElement.dataset.trackId = track.id;

    const mainElement = document.createElement("div");
    mainElement.className = "track-main";

    const topLineElement = document.createElement("div");
    topLineElement.className = "track-topline";

    const titleElement = document.createElement("p");
    titleElement.className = "track-title";
    titleElement.textContent = track.title;

    const artistElement = document.createElement("p");
    artistElement.className = "track-artist";
    artistElement.textContent = track.artistName;

    const idElement = document.createElement("p");
    idElement.className = "track-id";
    idElement.textContent = track.id;

    const actionsElement = document.createElement("div");
    actionsElement.className = "track-actions";

    const queueButtonElement = document.createElement("button");
    queueButtonElement.className = "queue-button";
    queueButtonElement.type = "button";
    queueButtonElement.dataset.action = "queue-track";
    queueButtonElement.dataset.trackId = track.id;
    queueButtonElement.textContent = "В очередь";

    const playButtonElement = document.createElement("button");
    playButtonElement.className = "play-button";
    playButtonElement.type = "button";
    playButtonElement.dataset.action = "play-track";
    playButtonElement.dataset.trackId = track.id;
    playButtonElement.dataset.title = track.title;
    playButtonElement.textContent = "Играть";

    topLineElement.append(titleElement, idElement);
    actionsElement.append(queueButtonElement, playButtonElement);
    mainElement.append(topLineElement, artistElement);
    trackElement.append(createArtworkElement(track), mainElement, actionsElement);
    listElement.append(trackElement);
  });

  updateCurrentTrackHighlights();
  syncTransportButtons();
}

async function initializeBridgeStatus() {
  if (typeof window.getAppInfo !== "function") {
    setBridgeStatus("Bridge недоступен", "Web UI открыт вне desktop shell.");
    return false;
  }

  try {
    // Это первый smoke-check того, что webview bind-функции реально зарегистрированы.
    const appInfo = normalizeBridgePayload(await window.getAppInfo());
    setBridgeStatus(appInfo.bridgeStatus, appInfo.applicationName);
    playbackBackendElement.textContent = `Playback backend: ${appInfo.playbackBackend || "неизвестен"}`;
    return true;
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setBridgeStatus("Ошибка bridge", errorMessage);
    return false;
  }
}

function hasPlaybackEnded(previousState, nextState) {
  if (!previousState) {
    return false;
  }

  return (
    nextState.completionToken > previousState.completionToken &&
    Boolean(nextState.trackId || previousState.trackId)
  );
}

async function runTransportCommand(command) {
  // На UI-уровне дополнительно сереализуем transport-команды,
  // чтобы пользователь не накладывал play/next/prev друг на друга слишком быстро.
  if (isTransportCommandInFlight) {
    return false;
  }

  isTransportCommandInFlight = true;
  syncTransportButtons();

  try {
    await command();
    return true;
  } finally {
    isTransportCommandInFlight = false;
    syncTransportButtons();
  }
}

async function playTrackById(trackId, title) {
  if (typeof window.playTrack !== "function") {
    setPlaybackStatus("Bridge недоступен", "Native playback API ещё не зарегистрирован.");
    return false;
  }

  if (isOfflineMode()) {
    setPlaybackStatus("Нет сети", "Для запуска нового трека требуется доступ к SoundCloud API.");
    return false;
  }

  return runTransportCommand(async () => {
    // Ошибки playback в момент старта могут приходить с лагом,
    // поэтому временно подавляем короткий transient-flash до следующего poll.
    isStartingPlayback = true;
    suppressPlaybackErrorsUntilMs = Date.now() + 2500;
    setPlaybackStatus("Подготовка...", `Запрашиваем native playback для: ${title}`);

    const response = normalizeBridgePayload(await window.playTrack({ trackId, title }));
    if (response?.ok === false) {
      isStartingPlayback = false;
      suppressPlaybackErrorsUntilMs = 0;
      throw new Error(response.message || "Не удалось запустить воспроизведение.");
    }

    currentTrackId = response.trackId || trackId;
    currentTrackTitle = response.trackTitle || title;
    markPendingTrackSwitch(currentTrackId, currentTrackTitle);

    await refreshQueueState();
    syncTransportButtons();
  });
}

async function playNextTrackIfAvailable() {
  if (isAutoAdvancing) {
    return;
  }

  // Auto-next и ручной next используют один и тот же transport flow.
  // Флаг нужен, чтобы completion-based auto-advance не запускался повторно поверх себя.
  isAutoAdvancing = true;

  try {
    if (isOfflineMode()) {
      setPlaybackStatus("Нет сети", "Следующий трек нельзя подготовить без доступа к SoundCloud API.");
      return;
    }

    if (typeof window.playNextTrack !== "function") {
      return;
    }

    const commandExecuted = await runTransportCommand(async () => {
      const response = normalizeBridgePayload(await window.playNextTrack());
      if (response?.ok === false) {
        throw new Error(response.message || "Следующий трек недоступен.");
      }

      currentTrackId = response.trackId || "";
      currentTrackTitle = response.trackTitle || "";
      isStartingPlayback = true;
      suppressPlaybackErrorsUntilMs = Date.now() + 2500;
      markPendingTrackSwitch(currentTrackId, currentTrackTitle);
      await refreshQueueState();
    });
    if (!commandExecuted) {
      return;
    }

    await refreshPlaybackState();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  } finally {
    isAutoAdvancing = false;
    syncTransportButtons();
  }
}

async function playCompletionFollowUpIfAvailable() {
  if (isAutoAdvancing) {
    return;
  }

  isAutoAdvancing = true;

  try {
    if (typeof window.playCompletionFollowUp !== "function") {
      return;
    }

    const commandExecuted = await runTransportCommand(async () => {
      const response = normalizeBridgePayload(await window.playCompletionFollowUp());
      if (response?.ok === false) {
        throw new Error(response.message || "Не удалось продолжить playback после окончания трека.");
      }

      if (response?.advanced === false) {
        return;
      }

      currentTrackId = response.trackId || "";
      currentTrackTitle = response.trackTitle || "";
      isStartingPlayback = true;
      suppressPlaybackErrorsUntilMs = Date.now() + 2500;
      markPendingTrackSwitch(currentTrackId, currentTrackTitle);
      await refreshQueueState();
    });
    if (!commandExecuted) {
      return;
    }

    await refreshPlaybackState();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  } finally {
    isAutoAdvancing = false;
    syncTransportButtons();
  }
}

async function refreshPlaybackState() {
  if (typeof window.getPlaybackState !== "function") {
    return;
  }

  try {
    // Polling playback state intentionally остаётся простым:
    // native-слой хранит реальное состояние, а UI только периодически его отражает.
    const response = normalizeBridgePayload(await window.getPlaybackState());
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось получить playback state.");
    }

    previousPlaybackState = latestPlaybackState;
    latestPlaybackState = {
      state: response.state || "idle",
      positionMs: response.positionMs || 0,
      durationMs: response.durationMs || 0,
      volumePercent:
        typeof response.volumePercent === "number"
          ? response.volumePercent
          : latestPlaybackState.volumePercent,
      completionToken: response.completionToken || 0,
      trackId: response.trackId || currentTrackId,
      trackTitle: response.trackTitle || currentTrackTitle,
    };

    currentTrackId = latestPlaybackState.trackId || currentTrackId;
    currentTrackTitle = latestPlaybackState.trackTitle || currentTrackTitle;
    latestQueueState.currentTrackId = response.trackId || latestQueueState.currentTrackId;
    latestQueueState.currentTrackTitle = response.trackTitle || latestQueueState.currentTrackTitle;
    latestQueueState.shuffleEnabled = Boolean(response.shuffleEnabled);
    latestQueueState.repeatMode = response.repeatMode || latestQueueState.repeatMode;
    latestQueueState.canPlayPrevious = Boolean(response.canPlayPrevious);
    latestQueueState.canPlayNext = Boolean(response.canPlayNext);
    hasLoadedTrack = latestPlaybackState.state !== "idle";
    isPlaybackPaused = latestPlaybackState.state === "paused";
    setPlaybackVisualState(latestPlaybackState.state);
    setShuffleState(latestQueueState.shuffleEnabled);
    setRepeatMode(latestQueueState.repeatMode);
    updateCurrentTrackHighlights();

    const isPlaybackActiveState = ["loading", "playing", "paused"].includes(latestPlaybackState.state);
    if (isPlaybackActiveState && latestPlaybackState.trackId) {
      isStartingPlayback = false;
      suppressPlaybackErrorsUntilMs = 0;
    }

    setPlaybackToggleState({
      disabled:
        latestPlaybackState.state === "idle" ||
        latestPlaybackState.state === "error" ||
        latestPlaybackState.state === "loading",
      paused: isPlaybackPaused,
    });
    if (!isProgressDragging) {
      setPlaybackProgress(latestPlaybackState.positionMs, latestPlaybackState.durationMs);
    }
    setVolumeLevel(latestPlaybackState.volumePercent);

    if (latestPlaybackState.state === "error") {
      if (isStartingPlayback && Date.now() < suppressPlaybackErrorsUntilMs) {
        return;
      }

      isStartingPlayback = false;
      setPlaybackStatus(
        mapPlaybackStateLabel(latestPlaybackState.state),
        response.errorMessage || "Неизвестная ошибка.",
      );
      return;
    }

    setPlaybackStatus(
      mapPlaybackStateLabel(latestPlaybackState.state),
      latestPlaybackState.trackTitle || "Трек ещё не выбран",
    );

    if (hasPlaybackEnded(previousPlaybackState, latestPlaybackState)) {
      // Переход после окончания трека идёт через native session-state:
      // именно он знает о queue, shuffle и repeat режимах.
      await playCompletionFollowUpIfAvailable();
    }
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackProgress(0, 0);
    setPlaybackStatus("Ошибка playback", errorMessage);
    setPlaybackVisualState("error");
  } finally {
    syncTransportButtons();
  }
}

async function loadFeaturedTracks() {
  if (typeof window.getFeaturedTracks !== "function") {
    featuredSummaryElement.textContent = "Bridge API популярных треков ещё не зарегистрирован.";
    return;
  }

  if (isOfflineMode()) {
    featuredSummaryElement.textContent = "Нет подключения к сети. Главная лента появится, когда интернет вернётся.";
    renderFeaturedTracks([], "Оффлайн-режим: популярные треки недоступны без сети.");
    return;
  }

  featuredSummaryElement.textContent = "Загружаем популярные треки...";

  try {
    // Главная витрина intentionally не смешивается с поиском:
    // она живёт в отдельной вкладке и отдельном списке треков.
    const response = normalizeBridgePayload(await window.getFeaturedTracks({ limit: 12 }));
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось загрузить популярные треки.");
    }

    const tracks = Array.isArray(response.tracks) ? response.tracks : [];
    featuredSummaryElement.textContent = `${response.title || "Популярное сейчас"}: ${tracks.length} совместимых трек(ов).`;
    renderFeaturedTracks(tracks, "SoundCloud не вернул совместимые треки для стартовой страницы.");
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    featuredSummaryElement.textContent = `Ошибка загрузки стартовой витрины: ${errorMessage}`;
    renderFeaturedTracks([], "Не удалось загрузить популярные треки.");
  }
}

async function handleSearchSubmit(event) {
  event.preventDefault();

  if (typeof window.searchTracks !== "function") {
    searchSummaryElement.textContent =
      "Bridge API не зарегистрирован. Проверь native-shell и регистрацию bind-функций.";
    return;
  }

  const query = searchQueryElement.value.trim();
  const limit = Number.parseInt(searchLimitElement.value, 10) || 24;
  const offset = Number.parseInt(searchOffsetElement.value, 10) || 0;

  if (isOfflineMode()) {
    searchSummaryElement.textContent = "Нет подключения к сети. Поиск по SoundCloud временно недоступен.";
    renderTracks([], "Оффлайн-режим: поиск временно недоступен.");
    return;
  }

  setActiveTab("search");
  searchSummaryElement.textContent = "Отправляем запрос в native bridge...";

  try {
    // UI передаёт в bridge только query/limit/offset и не знает деталей SoundCloud API.
    const response = normalizeBridgePayload(await window.searchTracks({ query, limit, offset }));
    if (response?.ok === false) {
      throw new Error(response.message || "Поиск через bridge завершился ошибкой.");
    }

    const tracks = Array.isArray(response.tracks) ? response.tracks : [];
    searchSummaryElement.textContent = `Результаты по "${response.query}": ${tracks.length} совместимых трек(ов), limit=${response.limit}, offset=${response.offset}.`;
    renderTracks(tracks, "По этому запросу SoundCloud не вернул совместимые треки.");
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    searchSummaryElement.textContent = `Ошибка запроса к bridge: ${errorMessage}`;
    renderTracks([], "Не удалось выполнить поиск.");
  }
}

async function handleTrackListClick(event) {
  // Один обработчик обслуживает и витрину, и поиск:
  // различается только то, из какого списка пришёл trackId.
  const trackButtonElement = event.target.closest("[data-action]");
  if (!trackButtonElement) {
    return;
  }

  const trackId = trackButtonElement.dataset.trackId || "";
  const track = findTrackById(trackId);
  if (!track) {
    return;
  }

  if (trackButtonElement.dataset.action === "queue-track") {
    try {
      await enqueueTrackRequest(track);
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      setPlaybackStatus("Ошибка очереди", errorMessage);
    }
    return;
  }

  try {
    await playTrackById(track.id, track.title);
    await refreshPlaybackState();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    hasLoadedTrack = false;
    isPlaybackPaused = false;
    setPlaybackToggleState({ disabled: true, paused: false });
    setPlaybackProgress(0, 0);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

async function handleQueueClick(event) {
  const removeButtonElement = event.target.closest("[data-action='remove-queued-track']");
  if (!removeButtonElement) {
    return;
  }

  const trackId = removeButtonElement.dataset.trackId || "";
  if (typeof window.removeQueuedTrack !== "function") {
    return;
  }

  try {
    const response = normalizeBridgePayload(await window.removeQueuedTrack({ trackId }));
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось удалить трек из очереди.");
    }

    await refreshQueueState();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка очереди", errorMessage);
  }
}

async function handlePlaybackToggle() {
  if (!hasLoadedTrack) {
    return;
  }

  // UI не хранит отдельно play/pause transport-state.
  // Он просто выбирает нужную bridge-функцию по последнему playback snapshot.
  const functionName = isPlaybackPaused ? "resumePlayback" : "pausePlayback";
  if (typeof window[functionName] !== "function") {
    setPlaybackStatus("Bridge недоступен", "Native playback toggle API ещё не зарегистрирован.");
    return;
  }

  try {
    const response = normalizeBridgePayload(await window[functionName]());
    if (response?.ok === false) {
      throw new Error(
        response.message ||
          (isPlaybackPaused
            ? "Не удалось возобновить воспроизведение."
            : "Не удалось поставить воспроизведение на паузу."),
      );
    }

    if (response.trackTitle) {
      currentTrackTitle = response.trackTitle;
    }

    await refreshPlaybackState();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

async function handlePrevTrack() {
  if (typeof window.playPreviousTrack !== "function") {
    return;
  }

  try {
    const commandExecuted = await runTransportCommand(async () => {
      const response = normalizeBridgePayload(await window.playPreviousTrack());
      if (response?.ok === false) {
        throw new Error(response.message || "Предыдущий трек недоступен.");
      }

      currentTrackId = response.trackId || "";
      currentTrackTitle = response.trackTitle || "";
      isStartingPlayback = true;
      suppressPlaybackErrorsUntilMs = Date.now() + 2500;
      markPendingTrackSwitch(currentTrackId, currentTrackTitle);
      await refreshQueueState();
    });
    if (!commandExecuted) {
      return;
    }

    await refreshPlaybackState();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

async function handleNextTrack() {
  await playNextTrackIfAvailable();
}

async function handleProgressSeek(event) {
  if (!canSeekPlayback()) {
    return;
  }

  const ratio = getProgressRatioFromClientX(event.clientX);
  const targetPositionMs = Math.round(latestPlaybackState.durationMs * ratio);

  try {
    // Seek отправляется в native player, а затем UI заново перечитывает playback state,
    // чтобы не расходиться с реальной позицией backend-а.
    const response = normalizeBridgePayload(await window.seekPlayback({ positionMs: targetPositionMs }));
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось перемотать трек.");
    }

    latestPlaybackState.positionMs = targetPositionMs;
    setPlaybackProgress(targetPositionMs, latestPlaybackState.durationMs);
    await refreshPlaybackState();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

function handleProgressPointerDown(event) {
  if (!canSeekPlayback()) {
    return;
  }

  isProgressDragging = true;
  progressDragPointerId = event.pointerId;
  playbackProgressTrackElement.setPointerCapture(event.pointerId);
  previewDraggedProgress(getProgressRatioFromClientX(event.clientX));
}

function handleProgressPointerMove(event) {
  if (!isProgressDragging || event.pointerId !== progressDragPointerId) {
    return;
  }

  previewDraggedProgress(getProgressRatioFromClientX(event.clientX));
}

function finishProgressDragging() {
  isProgressDragging = false;
  progressDragPointerId = null;
  playbackProgressTrackElement.classList.remove("is-dragging");
}

function handleProgressPointerCancel(event) {
  if (!isProgressDragging || event.pointerId !== progressDragPointerId) {
    return;
  }

  if (playbackProgressTrackElement.hasPointerCapture(event.pointerId)) {
    playbackProgressTrackElement.releasePointerCapture(event.pointerId);
  }

  finishProgressDragging();
  setPlaybackProgress(latestPlaybackState.positionMs, latestPlaybackState.durationMs);
}

async function handleProgressPointerUp(event) {
  if (!isProgressDragging || event.pointerId !== progressDragPointerId) {
    return;
  }

  const pointerId = progressDragPointerId;
  const targetPositionMs = Math.round(latestPlaybackState.durationMs * progressDragRatio);
  finishProgressDragging();

  if (playbackProgressTrackElement.hasPointerCapture(pointerId)) {
    playbackProgressTrackElement.releasePointerCapture(pointerId);
  }

  try {
    const response = normalizeBridgePayload(await window.seekPlayback({ positionMs: targetPositionMs }));
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось перемотать трек.");
    }

    latestPlaybackState.positionMs = targetPositionMs;
    setPlaybackProgress(targetPositionMs, latestPlaybackState.durationMs);
    await refreshPlaybackState();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

function scheduleVolumeSync(volumePercent) {
  if (volumeSyncTimer !== null) {
    window.clearTimeout(volumeSyncTimer);
  }

  volumeSyncTimer = window.setTimeout(() => {
    volumeSyncTimer = null;
    void applyVolumeChange(volumePercent).catch((error) => {
      const errorMessage = error instanceof Error ? error.message : String(error);
      setPlaybackStatus("Ошибка playback", errorMessage);
    });
  }, 80);
}

async function applyVolumeChange(volumePercent) {
  if (typeof window.setPlaybackVolume !== "function") {
    return;
  }

  const safeVolumePercent = Math.max(0, Math.min(100, Number(volumePercent) || 0));
  const response = normalizeBridgePayload(
    await window.setPlaybackVolume({ volumePercent: safeVolumePercent }),
  );
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось изменить громкость.");
  }

  latestPlaybackState.volumePercent =
    typeof response.volumePercent === "number" ? response.volumePercent : safeVolumePercent;
  setVolumeLevel(latestPlaybackState.volumePercent);
}

async function refreshEqualizerState() {
  if (typeof window.getEqualizerState !== "function") {
    return;
  }

  const response = normalizeBridgePayload(await window.getEqualizerState());
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось получить состояние эквалайзера.");
  }

  applyEqualizerStateSnapshot(response);
}

async function applyEqualizerEnabled(enabled) {
  if (typeof window.setEqualizerEnabled !== "function") {
    return;
  }

  const response = normalizeBridgePayload(await window.setEqualizerEnabled({ enabled }));
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось изменить состояние эквалайзера.");
  }

  applyEqualizerStateSnapshot(response);
}

async function applyEqualizerPreset(presetId) {
  if (typeof window.selectEqualizerPreset !== "function") {
    return;
  }

  const response = normalizeBridgePayload(await window.selectEqualizerPreset({ presetId }));
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось применить пресет эквалайзера.");
  }

  applyEqualizerStateSnapshot(response);
}

async function applyEqualizerBandGain(bandIndex, gainDb) {
  if (typeof window.setEqualizerBandGain !== "function") {
    return;
  }

  // Commit конкретной полосы идёт в native bridge, где уже пересчитываются
  // preset identity, headroom compensation и реальный DSP snapshot.
  const response = normalizeBridgePayload(
    await window.setEqualizerBandGain({
      bandIndex,
      gainDb,
    }),
  );
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось изменить полосу эквалайзера.");
  }

  applyEqualizerStateSnapshot(response);
}

async function applyEqualizerOutputGain(outputGainDb) {
  if (typeof window.setEqualizerOutputGain !== "function") {
    return;
  }

  // Level живёт в том же native snapshot-е, что и полосы:
  // UI не вычисляет headroom и не меняет DSP локально сам по себе.
  const response = normalizeBridgePayload(
    await window.setEqualizerOutputGain({
      outputGainDb,
    }),
  );
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось изменить общий уровень эквалайзера.");
  }

  applyEqualizerStateSnapshot(response);
}

async function requestEqualizerReset() {
  if (typeof window.resetEqualizer !== "function") {
    return;
  }

  const response = normalizeBridgePayload(await window.resetEqualizer());
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось сбросить эквалайзер.");
  }

  applyEqualizerStateSnapshot(response);
}

function scheduleEqualizerBandSync(bandIndex, gainDb) {
  const existingTimer = equalizerBandSyncTimers.get(bandIndex);
  if (existingTimer) {
    window.clearTimeout(existingTimer);
  }

  const timerId = window.setTimeout(() => {
    equalizerBandSyncTimers.delete(bandIndex);
    // Пока пользователь тянет полосу, применяем её с небольшим debounce:
    // так UI ощущается живым, но мы не спамим bridge лишними запросами на каждый pixel.
    void applyEqualizerBandGain(bandIndex, gainDb).catch((error) => {
      const errorMessage = error instanceof Error ? error.message : String(error);
      setPlaybackStatus("Ошибка EQ", errorMessage);
    });
  }, 70);

  equalizerBandSyncTimers.set(bandIndex, timerId);
}

function scheduleEqualizerOutputGainSync(outputGainDb) {
  if (outputGainSyncTimer !== null) {
    window.clearTimeout(outputGainSyncTimer);
  }

  outputGainSyncTimer = window.setTimeout(() => {
    outputGainSyncTimer = null;
    void applyEqualizerOutputGain(outputGainDb).catch((error) => {
      const errorMessage = error instanceof Error ? error.message : String(error);
      setPlaybackStatus("Ошибка EQ", errorMessage);
    });
  }, 70);
}

function cancelPendingEqualizerSync() {
  if (outputGainSyncTimer !== null) {
    window.clearTimeout(outputGainSyncTimer);
    outputGainSyncTimer = null;
  }

  equalizerBandSyncTimers.forEach((timerId) => {
    window.clearTimeout(timerId);
  });
  equalizerBandSyncTimers.clear();
}

function updateLocalEqualizerBandValue(bandIndex, gainDb) {
  const safeGainDb = clampEqualizerGainDb(gainDb);
  latestEqualizerState.bands = latestEqualizerState.bands.map((band) =>
    band.index === bandIndex
      ? { ...band, gainDb: safeGainDb }
      : band,
  );
  latestEqualizerState.activePresetId = deriveEqualizerPresetIdFromCurrentBands();
  setEqualizerStatusSummary();
  syncEqualizerPresetButtons();

  const sliderElement = equalizerBandsElement.querySelector(
    `.equalizer-slider[data-slider-kind="band"][data-band-index="${bandIndex}"]`,
  );
  if (sliderElement) {
    // Во время drag обновляем UI сразу локально, не дожидаясь roundtrip в C++.
    applyEqualizerSliderVisualState(sliderElement, safeGainDb);
  }

  scheduleEqualizerBandSync(bandIndex, safeGainDb);
}

function updateLocalEqualizerOutputGain(gainDb) {
  const safeGainDb = clampEqualizerGainDb(gainDb);
  latestEqualizerState.outputGainDb = safeGainDb;
  setEqualizerStatusSummary();

  const sliderElement = equalizerBandsElement.querySelector(
    `.equalizer-slider[data-slider-kind="output"]`,
  );
  if (sliderElement) {
    // Level preview показываем мгновенно, а реальное DSP применение приходит через debounce/commit.
    applyEqualizerSliderVisualState(sliderElement, safeGainDb);
  }

  scheduleEqualizerOutputGainSync(safeGainDb);
}

function handleVolumeInput(event) {
  const nextVolumePercent = Number.parseInt(event.target.value, 10) || 0;
  setVolumeLevel(nextVolumePercent);
  latestPlaybackState.volumePercent = nextVolumePercent;
  scheduleVolumeSync(nextVolumePercent);
}

async function handleVolumeChange(event) {
  const nextVolumePercent = Number.parseInt(event.target.value, 10) || 0;

  if (volumeSyncTimer !== null) {
    window.clearTimeout(volumeSyncTimer);
    volumeSyncTimer = null;
  }

  try {
    await applyVolumeChange(nextVolumePercent);
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

async function handleShuffleToggle() {
  if (typeof window.toggleShuffle !== "function") {
    return;
  }

  try {
    const response = normalizeBridgePayload(await window.toggleShuffle());
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось переключить shuffle.");
    }

    latestQueueState = {
      ...latestQueueState,
      currentTrackId: response.currentTrackId || latestQueueState.currentTrackId,
      currentTrackTitle: response.currentTrackTitle || latestQueueState.currentTrackTitle,
      queuedTracks: Array.isArray(response.queuedTracks) ? response.queuedTracks : latestQueueState.queuedTracks,
      shuffleEnabled: Boolean(response.shuffleEnabled),
      canPlayPrevious: Boolean(response.canPlayPrevious),
      canPlayNext: Boolean(response.canPlayNext),
    };
    setShuffleState(latestQueueState.shuffleEnabled);
    renderQueue();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

async function handleRepeatToggle() {
  if (typeof window.cycleRepeatMode !== "function") {
    return;
  }

  try {
    const response = normalizeBridgePayload(await window.cycleRepeatMode());
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось переключить repeat.");
    }

    latestQueueState = {
      ...latestQueueState,
      currentTrackId: response.currentTrackId || latestQueueState.currentTrackId,
      currentTrackTitle: response.currentTrackTitle || latestQueueState.currentTrackTitle,
      queuedTracks: Array.isArray(response.queuedTracks) ? response.queuedTracks : latestQueueState.queuedTracks,
      shuffleEnabled: Boolean(response.shuffleEnabled),
      repeatMode: response.repeatMode || "none",
      canPlayPrevious: Boolean(response.canPlayPrevious),
      canPlayNext: Boolean(response.canPlayNext),
    };
    setShuffleState(latestQueueState.shuffleEnabled);
    setRepeatMode(latestQueueState.repeatMode);
    renderQueue();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

async function handleEqualizerToggle() {
  try {
    cancelPendingEqualizerSync();
    await applyEqualizerEnabled(!latestEqualizerState.enabled);
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка EQ", errorMessage);
  }
}

async function handleEqualizerPresetClick(event) {
  const presetButtonElement = event.target.closest("[data-action='select-equalizer-preset']");
  if (!presetButtonElement) {
    return;
  }

  const presetId = presetButtonElement.dataset.presetId || "";
  try {
    cancelPendingEqualizerSync();
    await applyEqualizerPreset(presetId);
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка EQ", errorMessage);
  }
}

function getEqualizerSliderGainFromClientY(sliderElement, clientY) {
  const trackElement = sliderElement.querySelector(".equalizer-slider-track");
  if (!trackElement) {
    return 0;
  }

  const trackRect = trackElement.getBoundingClientRect();
  if (trackRect.height <= 0) {
    return 0;
  }

  const ratio = Math.min(1, Math.max(0, (clientY - trackRect.top) / trackRect.height));
  return clampEqualizerGainDb(12 - ratio * 24);
}

function previewEqualizerSliderValue(sliderElement, gainDb) {
  const sliderKind = sliderElement.dataset.sliderKind || "band";
  if (sliderKind === "output") {
    // Первый столбец EQ управляет не частотой, а общим уровнем обработанного сигнала.
    updateLocalEqualizerOutputGain(gainDb);
    return;
  }

  const bandIndex = Number.parseInt(sliderElement.dataset.bandIndex || "", 10);
  if (Number.isNaN(bandIndex)) {
    return;
  }

  updateLocalEqualizerBandValue(bandIndex, gainDb);
}

async function commitEqualizerSliderValue(sliderElement) {
  const sliderKind = sliderElement.dataset.sliderKind || "band";
  const gainDb = clampEqualizerGainDb(Number.parseFloat(sliderElement.dataset.valueDb || "0"));

  try {
    // На pointerup / keyboard commit делаем немедленный sync без ожидания debounce,
    // чтобы последнее положение ползунка точно оказалось в native state.
    if (sliderKind === "output") {
      if (outputGainSyncTimer !== null) {
        window.clearTimeout(outputGainSyncTimer);
        outputGainSyncTimer = null;
      }

      await applyEqualizerOutputGain(gainDb);
      return;
    }

    const bandIndex = Number.parseInt(sliderElement.dataset.bandIndex || "", 10);
    if (Number.isNaN(bandIndex)) {
      return;
    }

    const existingTimer = equalizerBandSyncTimers.get(bandIndex);
    if (existingTimer) {
      window.clearTimeout(existingTimer);
      equalizerBandSyncTimers.delete(bandIndex);
    }

    await applyEqualizerBandGain(bandIndex, gainDb);
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка EQ", errorMessage);
  }
}

function finishEqualizerDragging() {
  if (equalizerDragElement) {
    equalizerDragElement.classList.remove("is-dragging");
  }

  equalizerDragPointerId = null;
  equalizerDragElement = null;
}

function handleEqualizerSliderPointerDown(event) {
  const sliderElement = event.target.closest("[data-action='equalizer-slider']");
  if (!sliderElement || sliderElement.classList.contains("is-disabled")) {
    return;
  }

  event.preventDefault();
  equalizerDragPointerId = event.pointerId;
  equalizerDragElement = sliderElement;
  sliderElement.classList.add("is-dragging");
  sliderElement.setPointerCapture(event.pointerId);
  previewEqualizerSliderValue(sliderElement, getEqualizerSliderGainFromClientY(sliderElement, event.clientY));
}

function handleEqualizerSliderPointerMove(event) {
  if (!equalizerDragElement || event.pointerId !== equalizerDragPointerId) {
    return;
  }

  previewEqualizerSliderValue(
    equalizerDragElement,
    getEqualizerSliderGainFromClientY(equalizerDragElement, event.clientY),
  );
}

function handleEqualizerSliderPointerCancel(event) {
  if (!equalizerDragElement || event.pointerId !== equalizerDragPointerId) {
    return;
  }

  if (equalizerDragElement.hasPointerCapture(event.pointerId)) {
    equalizerDragElement.releasePointerCapture(event.pointerId);
  }

  finishEqualizerDragging();
  renderEqualizer();
}

async function handleEqualizerSliderPointerUp(event) {
  if (!equalizerDragElement || event.pointerId !== equalizerDragPointerId) {
    return;
  }

  const sliderElement = equalizerDragElement;
  if (sliderElement.hasPointerCapture(event.pointerId)) {
    sliderElement.releasePointerCapture(event.pointerId);
  }

  finishEqualizerDragging();
  await commitEqualizerSliderValue(sliderElement);
}

function handleEqualizerSliderKeyDown(event) {
  const sliderElement = event.target.closest("[data-action='equalizer-slider']");
  if (!sliderElement || sliderElement.classList.contains("is-disabled")) {
    return;
  }

  const currentValue = clampEqualizerGainDb(Number.parseFloat(sliderElement.dataset.valueDb || "0"));
  let nextValue = currentValue;

  switch (event.key) {
    case "ArrowUp":
    case "ArrowRight":
      nextValue = currentValue + 0.5;
      break;
    case "ArrowDown":
    case "ArrowLeft":
      nextValue = currentValue - 0.5;
      break;
    case "Home":
      nextValue = 12;
      break;
    case "End":
      nextValue = -12;
      break;
    default:
      return;
  }

  event.preventDefault();
  previewEqualizerSliderValue(sliderElement, nextValue);
  void commitEqualizerSliderValue(sliderElement);
}

function startPlaybackPolling() {
  if (playbackPollingTimer !== null) {
    clearInterval(playbackPollingTimer);
  }

  // Сейчас UI использует простой polling вместо событий из native-слоя.
  // Это не самая богатая модель, но она делает integration-path предсказуемым.
  playbackPollingTimer = window.setInterval(() => {
    void refreshPlaybackState();
  }, 1000);
}

async function initializePage() {
  // Инициализация идёт в безопасном порядке:
  // сначала приводим UI к базовому состоянию, затем проверяем bridge,
  // потом подтягиваем queue/playback state и только после этого стартуем витрину и polling.
  setPlaybackToggleState({ disabled: true, paused: false });
  setPlaybackProgress(0, 0);
  setVolumeLevel(100);
  setShuffleState(false);
  setRepeatMode("none");
  setPlaybackVisualState("idle");
  renderEqualizer();
  setActiveTab(activeTabId);
  renderFeaturedTracks([], "Здесь появятся популярные треки.");
  renderTracks([], "Результаты поиска появятся здесь.");
  renderQueue();

  const bridgeAvailable = await initializeBridgeStatus();
  if (!bridgeAvailable) {
    return;
  }

  await refreshQueueState();
  await refreshPlaybackState();
  try {
    await refreshEqualizerState();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    equalizerSummaryElement.textContent = `Не удалось инициализировать эквалайзер: ${errorMessage}`;
  }
  window.setTimeout(() => {
    void loadFeaturedTracks();
  }, 50);
  startPlaybackPolling();
}

searchFormElement.addEventListener("submit", handleSearchSubmit);
featuredListElement.addEventListener("click", (event) => {
  void handleTrackListClick(event);
});
tracksListElement.addEventListener("click", (event) => {
  void handleTrackListClick(event);
});
queueListElement.addEventListener("click", (event) => {
  void handleQueueClick(event);
});
prevTrackButtonElement.addEventListener("click", () => {
  void handlePrevTrack();
});
shuffleButtonElement.addEventListener("click", () => {
  void handleShuffleToggle();
});
repeatButtonElement.addEventListener("click", () => {
  void handleRepeatToggle();
});
equalizerToggleButtonElement.addEventListener("click", () => {
  void handleEqualizerToggle();
});
equalizerResetButtonElement.addEventListener("click", () => {
  cancelPendingEqualizerSync();
  void requestEqualizerReset().catch((error) => {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка EQ", errorMessage);
  });
});
equalizerPresetsElement.addEventListener("click", (event) => {
  void handleEqualizerPresetClick(event);
});
equalizerBandsElement.addEventListener("pointerdown", handleEqualizerSliderPointerDown);
equalizerBandsElement.addEventListener("pointermove", handleEqualizerSliderPointerMove);
equalizerBandsElement.addEventListener("pointerup", (event) => {
  void handleEqualizerSliderPointerUp(event);
});
equalizerBandsElement.addEventListener("pointercancel", handleEqualizerSliderPointerCancel);
equalizerBandsElement.addEventListener("keydown", handleEqualizerSliderKeyDown);
playbackToggleButtonElement.addEventListener("click", () => {
  void handlePlaybackToggle();
});
nextTrackButtonElement.addEventListener("click", () => {
  void handleNextTrack();
});
playbackProgressTrackElement.addEventListener("pointerdown", handleProgressPointerDown);
playbackProgressTrackElement.addEventListener("pointermove", handleProgressPointerMove);
playbackProgressTrackElement.addEventListener("pointerup", (event) => {
  void handleProgressPointerUp(event);
});
playbackProgressTrackElement.addEventListener("pointercancel", handleProgressPointerCancel);
volumeSliderElement.addEventListener("input", handleVolumeInput);
volumeSliderElement.addEventListener("change", (event) => {
  void handleVolumeChange(event);
});
loadFeaturedButtonElement.addEventListener("click", () => {
  setActiveTab("home");
  void loadFeaturedTracks();
});
tabButtonElements.forEach((tabButtonElement) => {
  tabButtonElement.addEventListener("click", () => {
    setActiveTab(tabButtonElement.dataset.tabTarget || "home");
  });
});

void initializePage();
