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
const playbackToggleButtonElement = document.getElementById("playback-toggle-button");
const loadFeaturedButtonElement = document.getElementById("load-featured-button");

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
  trackId: "",
  trackTitle: "",
};
let previousPlaybackState = null;
let isAutoAdvancing = false;

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

function renderTracks(tracks, emptyMessage) {
  currentRenderedTracks = Array.isArray(tracks) ? tracks : [];
  tracksListElement.replaceChildren();

  if (currentRenderedTracks.length === 0) {
    const emptyStateElement = document.createElement("li");
    emptyStateElement.className = "track-card track-card-empty";
    emptyStateElement.textContent = emptyMessage;
    tracksListElement.append(emptyStateElement);
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

    const playButtonElement = document.createElement("button");
    playButtonElement.className = "play-button";
    playButtonElement.type = "button";
    playButtonElement.dataset.action = "play-track";
    playButtonElement.dataset.trackId = track.id;
    playButtonElement.dataset.title = track.title;
    playButtonElement.textContent = "Играть";

    metaElement.append(badgeElement, idElement);
    actionsElement.append(playButtonElement);
    contentElement.append(metaElement, titleElement, artistElement, actionsElement);
    articleElement.append(createArtworkElement(track), contentElement);
    trackElement.append(articleElement);
    tracksListElement.append(trackElement);
  });
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

  const wasActive = ["loading", "playing", "paused"].includes(previousState.state);
  const isNowIdle = nextState.state === "idle";
  const completedTrack = nextState.durationMs > 0 && nextState.positionMs >= nextState.durationMs - 1000;

  return wasActive && isNowIdle && completedTrack && Boolean(nextState.trackId || previousState.trackId);
}

async function playTrackById(trackId, title) {
  if (typeof window.playTrack !== "function") {
    setPlaybackStatus("Bridge недоступен", "Native playback API ещё не зарегистрирован.");
    return false;
  }

  setPlaybackStatus("Подготовка...", `Запрашиваем native playback для: ${title}`);

  const response = normalizeBridgePayload(await window.playTrack({ trackId, title }));
  if (response?.ok === false) {
    throw new Error(response.message || "Не удалось запустить воспроизведение.");
  }

  currentTrackId = response.trackId || trackId;
  currentTrackTitle = response.trackTitle || title;
  return true;
}

async function playNextTrackIfAvailable() {
  if (isAutoAdvancing || currentRenderedTracks.length === 0 || !currentTrackId) {
    return;
  }

  const currentTrackIndex = currentRenderedTracks.findIndex((track) => track.id === currentTrackId);
  if (currentTrackIndex < 0 || currentTrackIndex + 1 >= currentRenderedTracks.length) {
    return;
  }

  const nextTrack = currentRenderedTracks[currentTrackIndex + 1];
  isAutoAdvancing = true;

  try {
    await playTrackById(nextTrack.id, nextTrack.title);
    await refreshPlaybackState();
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  } finally {
    isAutoAdvancing = false;
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
      trackId: response.trackId || currentTrackId,
      trackTitle: response.trackTitle || currentTrackTitle,
    };

    currentTrackId = latestPlaybackState.trackId || currentTrackId;
    currentTrackTitle = latestPlaybackState.trackTitle || currentTrackTitle;
    hasLoadedTrack = latestPlaybackState.state !== "idle";
    isPlaybackPaused = latestPlaybackState.state === "paused";

    setPlaybackToggleState({
      disabled:
        latestPlaybackState.state === "idle" ||
        latestPlaybackState.state === "error" ||
        latestPlaybackState.state === "loading",
      paused: isPlaybackPaused,
    });
    setPlaybackProgress(latestPlaybackState.positionMs, latestPlaybackState.durationMs);

    if (latestPlaybackState.state === "error") {
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
  }
}

async function loadFeaturedTracks() {
  if (typeof window.getFeaturedTracks !== "function") {
    searchSummaryElement.textContent = "Bridge API популярных треков ещё не зарегистрирован.";
    return;
  }

  searchSummaryElement.textContent = "Загружаем популярные треки...";

  try {
    const response = normalizeBridgePayload(await window.getFeaturedTracks({ limit: 12 }));
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось загрузить популярные треки.");
    }

    const tracks = Array.isArray(response.tracks) ? response.tracks : [];
    searchSummaryElement.textContent = `${response.title || "Популярное сейчас"}: ${tracks.length} трек(ов).`;
    renderTracks(tracks, "SoundCloud не вернул популярные треки для стартовой страницы.");
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

  searchSummaryElement.textContent = "Отправляем запрос в native bridge...";

  try {
    const response = normalizeBridgePayload(await window.searchTracks({ query, limit, offset }));
    if (response?.ok === false) {
      throw new Error(response.message || "Поиск через bridge завершился ошибкой.");
    }

    const tracks = Array.isArray(response.tracks) ? response.tracks : [];
    searchSummaryElement.textContent = `Результаты по "${response.query}": ${tracks.length} трек(ов), limit=${response.limit}, offset=${response.offset}.`;
    renderTracks(tracks, "По этому запросу SoundCloud не вернул доступные треки.");
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    searchSummaryElement.textContent = `Ошибка запроса к bridge: ${errorMessage}`;
    renderTracks([], "Не удалось выполнить поиск.");
  }
}

async function handleTrackListClick(event) {
  const playButtonElement = event.target.closest("[data-action='play-track']");
  if (!playButtonElement) {
    return;
  }

  const trackId = playButtonElement.dataset.trackId || "";
  const title = playButtonElement.dataset.title || "Без названия";

  try {
    await playTrackById(trackId, title);
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

  const bridgeAvailable = await initializeBridgeStatus();
  if (!bridgeAvailable) {
    return;
  }

  await refreshPlaybackState();
  await loadFeaturedTracks();
  startPlaybackPolling();
}

searchFormElement.addEventListener("submit", handleSearchSubmit);
tracksListElement.addEventListener("click", handleTrackListClick);
playbackToggleButtonElement.addEventListener("click", handlePlaybackToggle);
playbackProgressTrackElement.addEventListener("click", (event) => {
  void handleProgressSeek(event);
});
loadFeaturedButtonElement.addEventListener("click", () => {
  void loadFeaturedTracks();
});

void initializePage();
