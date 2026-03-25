# LilMusic

Desktop-клиент для SoundCloud с собственным UX, локальной пользовательской логикой и разделением на слои `UI -> Bridge -> Core -> Infrastructure`.

Проект находится в активной разработке. Текущая цель не в том, чтобы клонировать официальный SoundCloud, а в том, чтобы собрать альтернативный desktop-клиент с удобным интерфейсом, очередью воспроизведения, локальной библиотекой и расширяемой архитектурой.

## Что уже реализовано

- desktop shell на базе `webview`;
- web UI с вкладками `Главная / Поиск / Очередь`;
- поиск треков через публичный `api-v2.soundcloud.com`;
- стартовая лента популярных треков;
- native bridge между JavaScript и C++;
- playback state, progress bar, seek;
- очередь воспроизведения, `Prev / Next`, auto-next;
- локальная логика избранного;
- MSVC-сборка через `CMakePresets.json`.

## Важные ограничения

- авторизации SoundCloud нет;
- используются только публичные данные;
- доступ к SoundCloud идёт через `client_id`;
- избранное, очередь и будущие пользовательские данные относятся к приложению, а не к аккаунту SoundCloud;
- текущий активный playback backend в проекте: `Media Foundation`.

## Технологии

### Core

- C++20;
- CMake;
- Clean Architecture с разделением по слоям.

### UI

- HTML;
- CSS;
- JavaScript.

### Desktop shell

- [`webview`](https://github.com/webview/webview) как lightweight host для web UI.

### Networking

- WinHTTP.

### Database

- локальный слой `database` подготовлен под SQLite-репозиторий.

### Audio

- текущий рабочий backend: `Media Foundation / MFPlay`.

## Архитектура

Проект построен вокруг следующей схемы:

```text
UI (Web Layer)
↓
Bridge (JS ↔ C++)
↓
Core / Application Logic
↓
Infrastructure (API, Player, Database, Platform)
```

### Ключевые правила

- UI не знает о C++ реализации;
- `bridge` является единственной точкой связи web UI и native-слоя;
- `core` не зависит от UI;
- инфраструктурные детали изолированы в отдельных модулях;
- composition root находится в `app`.

## Структура проекта

```text
src/
  app/        # composition root и запуск приложения
  api/        # работа с SoundCloud API и разбор ответов
  bridge/     # JS ↔ C++ bridge
  core/       # доменные модели, use case-ы, session-state
  database/   # локальный persistence-слой
  platform/   # window shell и platform-specific интеграция
  player/     # native playback backend
  ui/         # HTML/CSS/JS интерфейс
```

## Ключевые модули

### `src/app`

Точка сборки всех зависимостей. Здесь создаются `api_client`, `audio_player`, use case-ы и `app_bridge`, после чего запускается окно приложения.

Главные файлы:

- [`src/app/src/main.cpp`](src/app/src/main.cpp)
- [`src/app/src/desktop_application.cpp`](src/app/src/desktop_application.cpp)

### `src/platform`

Отвечает за desktop shell: создание окна, загрузку UI-asset-ов и регистрацию bind-функций bridge в `webview`.

Главный файл:

- [`src/platform/src/window_controller.cpp`](src/platform/src/window_controller.cpp)

### `src/bridge`

Публичный native API для UI. Здесь JS-функции преобразуются в вызовы use case-ов, а ответы сериализуются обратно в JSON.

Главные файлы:

- [`src/bridge/src/app_bridge.cpp`](src/bridge/src/app_bridge.cpp)
- [`src/bridge/src/bridge_json_codec.cpp`](src/bridge/src/bridge_json_codec.cpp)

### `src/core`

Сердце приложения:

- доменные сущности;
- playback state;
- queue state;
- use case-ы;
- session-state воспроизведения.

Особенно важные файлы:

- [`src/core/include/soundcloud/core/domain/track.h`](src/core/include/soundcloud/core/domain/track.h)
- [`src/core/include/soundcloud/core/domain/playback_state.h`](src/core/include/soundcloud/core/domain/playback_state.h)
- [`src/core/include/soundcloud/core/domain/playback_queue_state.h`](src/core/include/soundcloud/core/domain/playback_queue_state.h)
- [`src/core/src/playback_session.cpp`](src/core/src/playback_session.cpp)
- [`src/core/src/search_tracks_use_case.cpp`](src/core/src/search_tracks_use_case.cpp)
- [`src/core/src/play_track_use_case.cpp`](src/core/src/play_track_use_case.cpp)

### `src/api`

Инфраструктурный адаптер для SoundCloud:

- запросы к публичному API;
- парсинг JSON-ответов;
- кэширование playback metadata;
- получение финального stream URL.

Главные файлы:

- [`src/api/src/soundcloud_api_client.cpp`](src/api/src/soundcloud_api_client.cpp)
- [`src/api/src/soundcloud_http_client.cpp`](src/api/src/soundcloud_http_client.cpp)
- [`src/api/src/soundcloud_track_search_response_parser.cpp`](src/api/src/soundcloud_track_search_response_parser.cpp)
- [`src/api/src/soundcloud_transcoding_response_parser.cpp`](src/api/src/soundcloud_transcoding_response_parser.cpp)

### `src/player`

Аудиоплеер и native backend.

Главные файлы:

- [`src/player/src/audio_player_service.cpp`](src/player/src/audio_player_service.cpp)
- [`src/player/src/media_foundation_audio_backend.cpp`](src/player/src/media_foundation_audio_backend.cpp)

Примечание: [`src/player/src/media_engine_audio_backend.cpp`](src/player/src/media_engine_audio_backend.cpp) сейчас не является активным playback-путём.

### `src/ui`

Web UI приложения:

- [`src/ui/index.html`](src/ui/index.html) — структура интерфейса;
- [`src/ui/styles.css`](src/ui/styles.css) — визуальный стиль;
- [`src/ui/app.js`](src/ui/app.js) — поведение UI и вызовы bridge API.

## Как собрать проект

### 1. Инициализация submodule

```bash
git submodule update --init --recursive
```

### 2. Конфигурация

В Windows PowerShell или `cmd`:

```powershell
.\scripts\configure_msvc.bat
```

или напрямую:

```powershell
cmake --preset msvc-x64-debug
```

### 3. Сборка

```powershell
.\scripts\build_msvc.bat
```

или напрямую:

```powershell
cmake --build --preset msvc-x64-debug
```

### 4. Запуск

```powershell
.\build\msvc-x64-debug\src\app\soundcloud_desktop.exe
```

## Как читать код

Если ты только входишь в проект, лучший маршрут такой:

1. [`src/app/src/main.cpp`](src/app/src/main.cpp)
2. [`src/app/src/desktop_application.cpp`](src/app/src/desktop_application.cpp)
3. [`src/platform/src/window_controller.cpp`](src/platform/src/window_controller.cpp)
4. [`src/bridge/src/app_bridge.cpp`](src/bridge/src/app_bridge.cpp)
5. [`src/ui/app.js`](src/ui/app.js)
6. [`src/core/src/playback_session.cpp`](src/core/src/playback_session.cpp)
7. [`src/core/src/search_tracks_use_case.cpp`](src/core/src/search_tracks_use_case.cpp)
8. [`src/core/src/play_track_use_case.cpp`](src/core/src/play_track_use_case.cpp)
9. [`src/api/src/soundcloud_api_client.cpp`](src/api/src/soundcloud_api_client.cpp)
10. [`src/api/src/soundcloud_http_client.cpp`](src/api/src/soundcloud_http_client.cpp)
11. [`src/player/src/audio_player_service.cpp`](src/player/src/audio_player_service.cpp)
12. [`src/player/src/media_foundation_audio_backend.cpp`](src/player/src/media_foundation_audio_backend.cpp)

Такой порядок даёт понимание по реальному потоку данных:

```text
UI action
-> bridge
-> use case
-> infrastructure
-> player / API
-> state back to UI
```

## Как мысленно разложить проект

При чтении каждого файла полезно задавать себе три вопроса:

1. Кто вызывает этот код?
2. От чего этот код зависит?
3. Это бизнес-логика, orchestration или инфраструктурная деталь?

Если держать эти три вопроса в голове, архитектура становится намного понятнее.

## Текущий UX-flow

### Поиск

`UI -> app_bridge -> search_tracks_use_case -> soundcloud_api_client -> SoundCloud API`

### Запуск трека

`UI -> app_bridge -> play_track_use_case -> resolve_stream_url -> audio_player_service -> Media Foundation backend`

### Очередь и next / prev

`UI -> app_bridge -> playback_session -> player state -> UI polling`

## Известные направления развития

- полноценная локальная библиотека на SQLite;
- user playlists;
- history;
- восстановление session-state;
- system tray;
- media keys;
- более асинхронная модель network I/O без блокирующих bridge-сценариев.

## Документы в репозитории

- [`soundcloud_desktop_prompt.md`](soundcloud_desktop_prompt.md) — основной документ с целями проекта и архитектурными правилами;
- [`music_app_style_guide.md`](music_app_style_guide.md) — визуальный ориентир для UI/UX.

## Статус

Сейчас проект уже можно собирать и запускать как desktop-приложение, но он всё ещё находится на стадии активного становления архитектуры и UX.
