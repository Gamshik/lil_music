export function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value));
}

export function clampEqualizerGainDb(gainDb: number): number {
  return Math.round(clamp(Number(gainDb) || 0, -12, 12) * 2) / 2;
}
