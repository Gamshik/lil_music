const bridgeStatusElement = document.getElementById("bridge-status");
const appNameElement = document.getElementById("app-name");
const searchFormElement = document.getElementById("search-form");
const searchQueryElement = document.getElementById("search-query");
const searchLimitElement = document.getElementById("search-limit");
const searchOffsetElement = document.getElementById("search-offset");
const searchSummaryElement = document.getElementById("search-summary");
const tracksListElement = document.getElementById("tracks-list");
const playbackBackendElement = document.getElementById("playback-backend");
const playbackStatusElement = document.getElementById("playback-status");
const currentTrackElement = document.getElementById("current-track");
const playbackProgressTrackElement = document.getElementById("playback-progress-track");
const playbackProgressFillElement = document.getElementById("playback-progress-fill");
const playbackPositionElement = document.getElementById("playback-position");
const playbackDurationElement = document.getElementById("playback-duration");
const prevTrackButtonElement = document.getElementById("prev-track-button");
const playbackToggleButtonElement = document.getElementById("playback-toggle-button");
const nextTrackButtonElement = document.getElementById("next-track-button");
const loadFeaturedButtonElement = document.getElementById("load-featured-button");
const queueCountElement = document.getElementById("queue-count");
const queueListElement = document.getElementById("queue-list");

let currentTrackTitle = "";
let currentTrackId = "";
let hasLoadedTrack = false;
let isPlaybackPaused = false;
let playbackPollingTimer = null;
let currentRenderedTracks = [];
let latestPlaybackState = {
  state: "idle",
  positionMs: 0,
  durationMs: 0,
  completionToken: 0,
  trackId: "",
  trackTitle: "",
};
let latestQueueState = {
  currentTrackId: "",
  currentTrackTitle: "",
  queuedTracks: [],
  canPlayPrevious: false,
  canPlayNext: false,
};
let previousPlaybackState = null;
let isAutoAdvancing = false;
let isStartingPlayback = false;
let isTransportCommandInFlight = false;
let suppressPlaybackErrorsUntilMs = 0;

function normalizeBridgePayload(payload) {
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
  playbackPositionElement.textContent = formatMilliseconds(safePosition);
  playbackDurationElement.textContent = formatMilliseconds(safeDuration);
}

function setPlaybackToggleState({ disabled, paused }) {
  playbackToggleButtonElement.disabled = disabled;
  playbackToggleButtonElement.textContent = paused ? "Продолжить" : "Пауза";
}

function markPendingTrackSwitch(trackId, trackTitle) {
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
  return (
    currentRenderedTracks.find((track) => track.id === trackId) ||
    latestQueueState.queuedTracks.find((track) => track.id === trackId) ||
    null
  );
}

function syncTransportButtons() {
  const shouldDisableTransportButtons =
    isTransportCommandInFlight || latestPlaybackState.state === "loading";
  prevTrackButtonElement.disabled =
    shouldDisableTransportButtons || !latestQueueState.canPlayPrevious;
  nextTrackButtonElement.disabled = shouldDisableTransportButtons || !latestQueueState.canPlayNext;
}

function renderQueue() {
  const queuedTracks = latestQueueState.queuedTracks;
  queueCountElement.textContent =
    queuedTracks.length === 0 ? "Очередь пуста" : `${queuedTracks.length} в очереди`;
  queueListElement.replaceChildren();

  if (queuedTracks.length === 0) {
    const emptyQueueElement = document.createElement("li");
    emptyQueueElement.className = "queue-item queue-item-empty";
    emptyQueueElement.textContent = "Следующим будет трек по текущему списку.";
    queueListElement.append(emptyQueueElement);
    syncTransportButtons();
    return;
  }

  queuedTracks.forEach((track) => {
    const queueItemElement = document.createElement("li");
    queueItemElement.className = "queue-item";

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

  syncTransportButtons();
}

async function refreshQueueState() {
  if (typeof window.getQueueState !== "function") {
    return;
  }

  const response = normalizeBridgePayload(await window.getQueueState());
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось получить состояние очереди.");
  }

  latestQueueState = {
    currentTrackId: response.currentTrackId || "",
    currentTrackTitle: response.currentTrackTitle || "",
    queuedTracks: Array.isArray(response.queuedTracks) ? response.queuedTracks : [],
    canPlayPrevious: Boolean(response.canPlayPrevious),
    canPlayNext: Boolean(response.canPlayNext),
  };
  renderQueue();
}

async function enqueueTrackRequest(track) {
  if (!track || !track.id || typeof window.enqueueTrack !== "function") {
    return;
  }

  const response = normalizeBridgePayload(await window.enqueueTrack({ trackId: track.id }));
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось добавить трек в очередь.");
  }

  await refreshQueueState();
}

function renderTracks(tracks, emptyMessage) {
  currentRenderedTracks = Array.isArray(tracks) ? tracks : [];
  tracksListElement.replaceChildren();

  if (currentRenderedTracks.length === 0) {
    const emptyStateElement = document.createElement("li");
    emptyStateElement.className = "track-card track-card-empty";
    emptyStateElement.textContent = emptyMessage;
    tracksListElement.append(emptyStateElement);
    syncTransportButtons();
    return;
  }

  currentRenderedTracks.forEach((track) => {
    const trackElement = document.createElement("li");
    trackElement.className = "track-card";

    const articleElement = document.createElement("article");
    articleElement.className = "track-card-body";

    const contentElement = document.createElement("div");
    contentElement.className = "track-content";

    const metaElement = document.createElement("div");
    metaElement.className = "track-meta";

    const badgeElement = document.createElement("span");
    badgeElement.className = "track-badge";
    badgeElement.textContent = "SoundCloud";

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

    metaElement.append(badgeElement, idElement);
    actionsElement.append(queueButtonElement, playButtonElement);
    contentElement.append(metaElement, titleElement, artistElement, actionsElement);
    articleElement.append(createArtworkElement(track), contentElement);
    trackElement.append(articleElement);
    tracksListElement.append(trackElement);
  });

  syncTransportButtons();
}

async function initializeBridgeStatus() {
  if (typeof window.getAppInfo !== "function") {
    setBridgeStatus("Bridge недоступен", "Web UI открыт вне desktop shell.");
    return false;
  }

  try {
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

async function refreshPlaybackState() {
  if (typeof window.getPlaybackState !== "function") {
    return;
  }

  try {
    const response = normalizeBridgePayload(await window.getPlaybackState());
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось получить playback state.");
    }

    previousPlaybackState = latestPlaybackState;
    latestPlaybackState = {
      state: response.state || "idle",
      positionMs: response.positionMs || 0,
      durationMs: response.durationMs || 0,
      completionToken: response.completionToken || 0,
      trackId: response.trackId || currentTrackId,
      trackTitle: response.trackTitle || currentTrackTitle,
    };

    currentTrackId = latestPlaybackState.trackId || currentTrackId;
    currentTrackTitle = latestPlaybackState.trackTitle || currentTrackTitle;
    latestQueueState.currentTrackId = response.trackId || latestQueueState.currentTrackId;
    latestQueueState.currentTrackTitle = response.trackTitle || latestQueueState.currentTrackTitle;
    latestQueueState.canPlayPrevious = Boolean(response.canPlayPrevious);
    latestQueueState.canPlayNext = Boolean(response.canPlayNext);
    hasLoadedTrack = latestPlaybackState.state !== "idle";
    isPlaybackPaused = latestPlaybackState.state === "paused";

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
    setPlaybackProgress(latestPlaybackState.positionMs, latestPlaybackState.durationMs);

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
      await playNextTrackIfAvailable();
    }
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackProgress(0, 0);
    setPlaybackStatus("Ошибка playback", errorMessage);
  } finally {
    syncTransportButtons();
  }
}

async function loadFeaturedTracks() {
  if (typeof window.getFeaturedTracks !== "function") {
    searchSummaryElement.textContent = "Bridge API популярных треков ещё не зарегистрирован.";
    return;
  }

  if (isOfflineMode()) {
    searchSummaryElement.textContent = "Нет подключения к сети. Стартовая витрина будет загружена, когда интернет появится.";
    renderTracks([], "Оффлайн-режим: популярные треки недоступны без сети.");
    return;
  }

  searchSummaryElement.textContent = "Загружаем популярные треки...";

  try {
    const response = normalizeBridgePayload(await window.getFeaturedTracks({ limit: 12 }));
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось загрузить популярные треки.");
    }

    const tracks = Array.isArray(response.tracks) ? response.tracks : [];
    searchSummaryElement.textContent = `${response.title || "Популярное сейчас"}: ${tracks.length} совместимых трек(ов).`;
    renderTracks(tracks, "SoundCloud не вернул совместимые треки для стартовой страницы.");
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    searchSummaryElement.textContent = `Ошибка загрузки стартовой витрины: ${errorMessage}`;
    renderTracks([], "Не удалось загрузить популярные треки.");
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

  searchSummaryElement.textContent = "Отправляем запрос в native bridge...";

  try {
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
  if (
    !hasLoadedTrack ||
    latestPlaybackState.durationMs <= 0 ||
    typeof window.seekPlayback !== "function"
  ) {
    return;
  }

  const trackRect = playbackProgressTrackElement.getBoundingClientRect();
  const ratio = Math.min(1, Math.max(0, (event.clientX - trackRect.left) / trackRect.width));
  const targetPositionMs = Math.round(latestPlaybackState.durationMs * ratio);

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

function startPlaybackPolling() {
  if (playbackPollingTimer !== null) {
    clearInterval(playbackPollingTimer);
  }

  playbackPollingTimer = window.setInterval(() => {
    void refreshPlaybackState();
  }, 1000);
}

async function initializePage() {
  setPlaybackToggleState({ disabled: true, paused: false });
  setPlaybackProgress(0, 0);
  renderQueue();

  const bridgeAvailable = await initializeBridgeStatus();
  if (!bridgeAvailable) {
    return;
  }

  await refreshQueueState();
  await refreshPlaybackState();
  window.setTimeout(() => {
    void loadFeaturedTracks();
  }, 50);
  startPlaybackPolling();
}

searchFormElement.addEventListener("submit", handleSearchSubmit);
tracksListElement.addEventListener("click", (event) => {
  void handleTrackListClick(event);
});
queueListElement.addEventListener("click", (event) => {
  void handleQueueClick(event);
});
prevTrackButtonElement.addEventListener("click", () => {
  void handlePrevTrack();
});
playbackToggleButtonElement.addEventListener("click", () => {
  void handlePlaybackToggle();
});
nextTrackButtonElement.addEventListener("click", () => {
  void handleNextTrack();
});
playbackProgressTrackElement.addEventListener("click", (event) => {
  void handleProgressSeek(event);
});
loadFeaturedButtonElement.addEventListener("click", () => {
  void loadFeaturedTracks();
});

void initializePage();
