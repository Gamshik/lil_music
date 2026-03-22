const bridgeStatusElement = document.getElementById("bridge-status");
const appNameElement = document.getElementById("app-name");
const searchFormElement = document.getElementById("search-form");
const searchQueryElement = document.getElementById("search-query");
const searchSummaryElement = document.getElementById("search-summary");
const tracksListElement = document.getElementById("tracks-list");
const playbackBackendElement = document.getElementById("playback-backend");
const playbackStatusElement = document.getElementById("playback-status");
const currentTrackElement = document.getElementById("current-track");
const pausePlaybackButtonElement = document.getElementById("pause-playback-button");

let currentTrackTitle = "";

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

function renderTracks(tracks) {
  tracksListElement.replaceChildren();

  if (tracks.length === 0) {
    const emptyStateElement = document.createElement("li");
    emptyStateElement.className = "track-card track-card-empty";
    emptyStateElement.textContent =
      "Native bridge ответил корректно, но адаптер SoundCloud пока не вернул треки.";
    tracksListElement.append(emptyStateElement);
    return;
  }

  tracks.forEach((track) => {
    const trackElement = document.createElement("li");
    trackElement.className = "track-card";

    const articleElement = document.createElement("article");
    const actionsElement = document.createElement("div");
    const titleElement = document.createElement("p");
    const artistElement = document.createElement("p");
    const idElement = document.createElement("p");
    const playButtonElement = document.createElement("button");

    titleElement.className = "track-title";
    artistElement.className = "track-artist";
    idElement.className = "track-id";
    actionsElement.className = "track-actions";
    playButtonElement.className = "play-button";
    playButtonElement.type = "button";
    playButtonElement.dataset.action = "play-track";
    playButtonElement.dataset.trackId = track.id;
    playButtonElement.dataset.title = track.title;
    playButtonElement.textContent = "Играть";

    titleElement.textContent = track.title;
    artistElement.textContent = track.artistName;
    idElement.textContent = "Playback URL будет разрешён через native API по track URN.";

    actionsElement.append(playButtonElement);
    articleElement.append(titleElement, artistElement, idElement, actionsElement);
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
  searchSummaryElement.textContent = "Отправляем запрос в native bridge...";

  try {
    const response = normalizeBridgePayload(await window.searchTracks({ query }));
    if (response?.ok === false) {
      throw new Error(response.message || "Поиск через bridge завершился ошибкой.");
    }

    const tracks = Array.isArray(response.tracks) ? response.tracks : [];

    searchSummaryElement.textContent = `Native bridge обработал запрос "${response.query}" и вернул ${tracks.length} трек(ов).`;
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
    pausePlaybackButtonElement.disabled = false;
    setPlaybackStatus("Играет", currentTrackTitle);
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

async function handlePausePlayback() {
  if (typeof window.pausePlayback !== "function") {
    setPlaybackStatus("Bridge недоступен", "Native pause API ещё не зарегистрирован.");
    return;
  }

  try {
    const response = normalizeBridgePayload(await window.pausePlayback());
    if (response?.ok === false) {
      throw new Error(response.message || "Не удалось поставить воспроизведение на паузу.");
    }

    setPlaybackStatus("Пауза", currentTrackTitle || "Воспроизведение приостановлено");
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : String(error);
    setPlaybackStatus("Ошибка playback", errorMessage);
  }
}

searchFormElement.addEventListener("submit", handleSearchSubmit);
tracksListElement.addEventListener("click", handleTrackListClick);
pausePlaybackButtonElement.addEventListener("click", handlePausePlayback);
initializeBridgeStatus();
