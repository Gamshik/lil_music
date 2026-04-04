import type { EqualizerPreset, EqualizerPresetId, EqualizerStatus, PlaybackStatus, RepeatMode } from "../types/models";

export function formatMilliseconds(milliseconds: number): string {
  const totalSeconds = Math.max(0, Math.floor(milliseconds / 1000));
  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;
  return `${minutes}:${String(seconds).padStart(2, "0")}`;
}

export function formatEqualizerGainDb(gainDb: number): string {
  const safeGainDb = Number(gainDb) || 0;
  return `${safeGainDb > 0 ? "+" : ""}${safeGainDb.toFixed(1)} dB`;
}

export function formatEqualizerFrequencyLabel(centerFrequencyHz: number): string {
  const safeFrequencyHz = Number(centerFrequencyHz) || 0;
  if (safeFrequencyHz >= 1000) {
    const valueInKhz = safeFrequencyHz / 1000;
    return `${Number.isInteger(valueInKhz) ? valueInKhz.toFixed(0) : valueInKhz.toFixed(1)}k`;
  }

  return `${Math.round(safeFrequencyHz)}`;
}

export function mapPlaybackStateLabel(state: PlaybackStatus): string {
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

export function mapEqualizerStatusLabel(status: EqualizerStatus): string {
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

export function describeRepeatMode(mode: RepeatMode): string {
  if (mode === "playlist") {
    return "Повтор списка";
  }

  if (mode === "track") {
    return "Повтор трека";
  }

  return "Повтор выключен";
}

export function getEqualizerPresetTitle(
  presets: EqualizerPreset[],
  presetId: EqualizerPresetId,
): string {
  return presets.find((preset) => preset.id === presetId)?.title || "Flat";
}
