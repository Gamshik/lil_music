import { useEffect, useRef, useState } from "react";
import { clamp } from "@utils/math";
import { formatMilliseconds } from "@utils/formatters";
import styles from "./ProgressBar.module.scss";

interface ProgressBarProps {
  positionMs: number;
  durationMs: number;
  disabled: boolean;
  onSeek: (positionMs: number) => Promise<void>;
}

export function ProgressBar({ positionMs, durationMs, disabled, onSeek }: ProgressBarProps) {
  const trackRef = useRef<HTMLDivElement | null>(null);
  const dragPointerIdRef = useRef<number | null>(null);
  const [previewRatio, setPreviewRatio] = useState<number | null>(null);

  useEffect(() => {
    if (disabled) {
      setPreviewRatio(null);
    }
  }, [disabled]);

  const currentRatio =
    previewRatio ?? (durationMs > 0 ? clamp(positionMs / durationMs, 0, 1) : 0);
  const previewPositionMs = Math.round(durationMs * currentRatio);

  const getRatioFromClientX = (clientX: number) => {
    const trackElement = trackRef.current;
    if (!trackElement) {
      return 0;
    }

    const rect = trackElement.getBoundingClientRect();
    if (rect.width <= 0) {
      return 0;
    }

    return clamp((clientX - rect.left) / rect.width, 0, 1);
  };

  const finishDrag = () => {
    dragPointerIdRef.current = null;
    setPreviewRatio(null);
  };

  const handlePointerDown: React.PointerEventHandler<HTMLDivElement> = (event) => {
    if (disabled) {
      return;
    }

    dragPointerIdRef.current = event.pointerId;
    event.currentTarget.setPointerCapture(event.pointerId);
    setPreviewRatio(getRatioFromClientX(event.clientX));
  };

  const handlePointerMove: React.PointerEventHandler<HTMLDivElement> = (event) => {
    if (dragPointerIdRef.current !== event.pointerId) {
      return;
    }

    setPreviewRatio(getRatioFromClientX(event.clientX));
  };

  const handlePointerCancel: React.PointerEventHandler<HTMLDivElement> = (event) => {
    if (dragPointerIdRef.current !== event.pointerId) {
      return;
    }

    if (event.currentTarget.hasPointerCapture(event.pointerId)) {
      event.currentTarget.releasePointerCapture(event.pointerId);
    }

    finishDrag();
  };

  const handlePointerUp: React.PointerEventHandler<HTMLDivElement> = async (event) => {
    if (dragPointerIdRef.current !== event.pointerId) {
      return;
    }

    const targetRatio = getRatioFromClientX(event.clientX);
    if (event.currentTarget.hasPointerCapture(event.pointerId)) {
      event.currentTarget.releasePointerCapture(event.pointerId);
    }

    finishDrag();
    await onSeek(Math.round(durationMs * targetRatio));
  };

  return (
    <div className={styles.progress}>
      <div
        ref={trackRef}
        className={`${styles.track} ${previewRatio !== null ? styles.isDragging : ""}`}
        role="slider"
        aria-label="Playback progress"
        aria-valuemin={0}
        aria-valuemax={100}
        aria-valuenow={Math.round(currentRatio * 100)}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={(event) => void handlePointerUp(event)}
        onPointerCancel={handlePointerCancel}
      >
        <div className={styles.fill} style={{ width: `${currentRatio * 100}%` }} />
        <div className={styles.thumb} style={{ left: `${currentRatio * 100}%` }} aria-hidden="true" />
      </div>

      <div className={styles.time}>
        <span>{formatMilliseconds(previewPositionMs)}</span>
        <span>{formatMilliseconds(durationMs)}</span>
      </div>
    </div>
  );
}
