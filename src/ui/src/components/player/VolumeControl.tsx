import { useEffect, useRef, useState } from "react";
import { clamp } from "@utils/math";
import styles from "./VolumeControl.module.scss";

interface VolumeControlProps {
  value: number;
  onChange: (volumePercent: number) => Promise<void>;
}

export function VolumeControl({ value, onChange }: VolumeControlProps) {
  const [draftValue, setDraftValue] = useState(value);
  const timerRef = useRef<number | null>(null);

  useEffect(() => {
    setDraftValue(value);
  }, [value]);

  const flushVolume = async (nextValue: number) => {
    if (timerRef.current !== null) {
      window.clearTimeout(timerRef.current);
      timerRef.current = null;
    }

    await onChange(clamp(nextValue, 0, 100));
  };

  const scheduleVolume = (nextValue: number) => {
    if (timerRef.current !== null) {
      window.clearTimeout(timerRef.current);
    }

    timerRef.current = window.setTimeout(() => {
      timerRef.current = null;
      void onChange(clamp(nextValue, 0, 100));
    }, 80);
  };

  return (
    <label className={styles.volumeControl}>
      <span className={styles.label}>Громкость</span>
      <input
        className={styles.slider}
        type="range"
        min="0"
        max="100"
        step="1"
        value={draftValue}
        aria-label="Громкость воспроизведения"
        onChange={(event) => {
          const nextValue = Number(event.target.value);
          setDraftValue(nextValue);
          scheduleVolume(nextValue);
        }}
        onPointerUp={() => {
          void flushVolume(draftValue);
        }}
      />
      <span className={styles.value}>{draftValue}%</span>
    </label>
  );
}
