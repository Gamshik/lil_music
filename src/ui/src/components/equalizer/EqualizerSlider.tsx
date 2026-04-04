import { useRef } from "react";
import { clampEqualizerGainDb } from "@utils/math";
import { formatEqualizerGainDb } from "@utils/formatters";
import styles from "./EqualizerSlider.module.scss";

interface EqualizerSliderProps {
  label: string;
  valueDb: number;
  disabled: boolean;
  output?: boolean;
  onPreview: (gainDb: number) => void;
  onCommit: (gainDb: number) => Promise<void>;
}

export function EqualizerSlider({
  label,
  valueDb,
  disabled,
  output = false,
  onPreview,
  onCommit,
}: EqualizerSliderProps) {
  const sliderRef = useRef<HTMLDivElement | null>(null);
  const pointerIdRef = useRef<number | null>(null);

  const normalized = (12 - valueDb) / 24;
  const thumbTopPercent = Math.min(100, Math.max(0, normalized * 100));

  const getGainFromClientY = (clientY: number) => {
    const sliderElement = sliderRef.current;
    if (!sliderElement) {
      return 0;
    }

    const rect = sliderElement.getBoundingClientRect();
    if (rect.height <= 0) {
      return 0;
    }

    const ratio = Math.min(1, Math.max(0, (clientY - rect.top) / rect.height));
    return clampEqualizerGainDb(12 - ratio * 24);
  };

  const handlePointerDown: React.PointerEventHandler<HTMLDivElement> = (event) => {
    if (disabled) {
      return;
    }

    pointerIdRef.current = event.pointerId;
    event.currentTarget.setPointerCapture(event.pointerId);
    onPreview(getGainFromClientY(event.clientY));
  };

  const handlePointerMove: React.PointerEventHandler<HTMLDivElement> = (event) => {
    if (pointerIdRef.current !== event.pointerId) {
      return;
    }

    onPreview(getGainFromClientY(event.clientY));
  };

  const releasePointer = (event: React.PointerEvent<HTMLDivElement>) => {
    if (event.currentTarget.hasPointerCapture(event.pointerId)) {
      event.currentTarget.releasePointerCapture(event.pointerId);
    }

    pointerIdRef.current = null;
  };

  const handlePointerCancel: React.PointerEventHandler<HTMLDivElement> = (event) => {
    if (pointerIdRef.current !== event.pointerId) {
      return;
    }

    releasePointer(event);
  };

  const handlePointerUp: React.PointerEventHandler<HTMLDivElement> = async (event) => {
    if (pointerIdRef.current !== event.pointerId) {
      return;
    }

    const nextValue = getGainFromClientY(event.clientY);
    releasePointer(event);
    await onCommit(nextValue);
  };

  const handleKeyDown: React.KeyboardEventHandler<HTMLDivElement> = (event) => {
    let nextValue = valueDb;

    switch (event.key) {
      case "ArrowUp":
      case "ArrowRight":
        nextValue += 0.5;
        break;
      case "ArrowDown":
      case "ArrowLeft":
        nextValue -= 0.5;
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
    const safeValue = clampEqualizerGainDb(nextValue);
    onPreview(safeValue);
    void onCommit(safeValue);
  };

  return (
    <div className={styles.band}>
      <span className={styles.value}>{formatEqualizerGainDb(valueDb)}</span>
      <div
        ref={sliderRef}
        className={`${styles.slider} ${output ? styles.isOutput : ""} ${disabled ? styles.isDisabled : ""}`}
        role="slider"
        aria-label={`${label}, ${formatEqualizerGainDb(valueDb)}`}
        aria-valuemin={-12}
        aria-valuemax={12}
        aria-valuenow={valueDb}
        aria-valuetext={formatEqualizerGainDb(valueDb)}
        tabIndex={disabled ? -1 : 0}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={(event) => void handlePointerUp(event)}
        onPointerCancel={handlePointerCancel}
        onKeyDown={handleKeyDown}
      >
        <div className={styles.track}>
          <div className={styles.rail} />
          <div className={styles.zeroLine} />
          <div
            className={styles.fill}
            style={
              valueDb >= 0
                ? { top: `${thumbTopPercent}%`, height: `${50 - thumbTopPercent}%` }
                : { top: "50%", height: `${thumbTopPercent - 50}%` }
            }
          />
          <div className={styles.thumb} style={{ top: `${thumbTopPercent}%` }} />
        </div>
      </div>
      <span className={styles.label}>{label}</span>
    </div>
  );
}
