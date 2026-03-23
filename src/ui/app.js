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
const playbackToggleButtonElement = document.getElementById("playback-toggle-button");

let currentTrackTitle = "";
let hasLoadedTrack = false;
let isPlaybackPaused = false;

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

function setPlaybackToggleState({ disabled, paused }) {
  playbackToggleButtonElement.disabled = disabled;
  playbackToggleButtonElement.textContent = paused ? "Продолжить" : "Пауза";
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

function renderTracks(tracks) {
  tracksListElement.replaceChildren();

  if (tracks.length === 0) {
    const emptyStateElement = document.createElement("li");
    emptyStateElement.className = "track-card track-card-empty";
    emptyStateElement.textContent = "По этому запросу SoundCloud не вернул доступные треки.";
    tracksListElement.append(emptyStateElement);
    return;
  }

  tracks.forEach((track) => {
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
    return;
  }

  try {
    const appInfo = normalizeBridgePayload(await window.getAppInfo());
    setBridgeStatus(appInfo.bridgeStatus, appInfo.applicationName);
    playbackBackendElement.textContent = `Playback backend: ${appInfo.playbackBackend || "неизвестен"}`;
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setBridgeStatus("Ошибка bridge", errorMessage);
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
    searchSummaryElement.textContent = `Native bridge обработал запрос "${response.query}" и вернул ${tracks.length} трек(ов) при limit=${response.limit} и offset=${response.offset}.`;
    renderTracks(tracks);
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    searchSummaryElement.textContent = `Ошибка запроса к bridge: ${errorMessage}`;
    renderTracks([]);
  }
}

async function handleTrackListClick(event) {
  const playButtonElement = event.target.closest("[data-action='play-track']");
  if (!playButtonElement) {
    return;
  }

  if (typeof window.playTrack !== "function") {
    setPlaybackStatus("Bridge недоступен", "Native playback API ещё не зарегистрирован.");
    return;
  }

  const trackId = playButtonElement.dataset.trackId || "";
  const title = playButtonElement.dataset.title || "Без названия";
  setPlaybackStatus("Подготовка...", `Запрашиваем native playback для: ${title}`);

  try {
    const response = normalizeBridgePayload(await window.playTrack({ trackId, title }));
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось запустить воспроизведение.");
    }

    currentTrackTitle = response.trackTitle || title;
    hasLoadedTrack = true;
    isPlaybackPaused = false;
    setPlaybackToggleState({ disabled: false, paused: false });
    setPlaybackStatus("Запуск...", currentTrackTitle);
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    hasLoadedTrack = false;
    isPlaybackPaused = false;
    setPlaybackToggleState({ disabled: true, paused: false });
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

    isPlaybackPaused = !isPlaybackPaused;
    setPlaybackToggleState({ disabled: false, paused: isPlaybackPaused });
    setPlaybackStatus(
      isPlaybackPaused ? "Пауза" : "Воспроизведение",
      currentTrackTitle || "Текущий трек недоступен",
    );
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

searchFormElement.addEventListener("submit", handleSearchSubmit);
tracksListElement.addEventListener("click", handleTrackListClick);
playbackToggleButtonElement.addEventListener("click", handlePlaybackToggle);
setPlaybackToggleState({ disabled: true, paused: false });
initializeBridgeStatus();
