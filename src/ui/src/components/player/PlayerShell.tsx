import type { AudioOutputDevice, PlaybackState, PlaybackStatus, QueueState } from "@models/models";
import { describeRepeatMode } from "@utils/formatters";
import { OutputDeviceControl } from "./OutputDeviceControl";
import { ProgressBar } from "./ProgressBar";
import { VolumeControl } from "./VolumeControl";
import styles from "./PlayerShell.module.scss";

interface PlayerShellProps {
  playbackBackend: string;
  playbackState: PlaybackState;
  playbackStatusLabel: string;
  playbackStatusDetails: string;
  currentTrackTitle: string;
  queueState: QueueState;
  audioOutputDevices: AudioOutputDevice[];
  isTransportBusy: boolean;
  onTogglePlayback: () => Promise<void>;
  onPlayPrevious: () => Promise<void>;
  onPlayNext: () => Promise<void>;
  onSeek: (positionMs: number) => Promise<void>;
  onVolumeChange: (volumePercent: number) => Promise<void>;
  onRefreshOutputDevices: () => Promise<void>;
  onSelectOutputDevice: (deviceId: string) => Promise<void>;
  onToggleShuffle: () => Promise<void>;
  onCycleRepeatMode: () => Promise<void>;
}

function getVisualState(
  playbackState: PlaybackState,
  statusLabel: string,
): PlaybackStatus {
  return statusLabel === "Ошибка playback" ? "error" : playbackState.state;
}

export function PlayerShell({
  playbackBackend,
  playbackState,
  playbackStatusLabel,
  playbackStatusDetails,
  currentTrackTitle,
  queueState,
  audioOutputDevices,
  isTransportBusy,
  onTogglePlayback,
  onPlayPrevious,
  onPlayNext,
  onSeek,
  onVolumeChange,
  onRefreshOutputDevices,
  onSelectOutputDevice,
  onToggleShuffle,
  onCycleRepeatMode,
}: PlayerShellProps) {
  const effectiveVisualState = getVisualState(playbackState, playbackStatusLabel);
  const transportDisabled = isTransportBusy || playbackState.state === "loading";

  return (
    <section className={styles.playerShell} data-player-shell="true">
      <div className={styles.playerCopy}>
        <div className={styles.playerHead}>
          <div>
            <p className={styles.eyebrow}>Now playing</p>
            <strong className={styles.playbackStatus}>{playbackStatusLabel}</strong>
          </div>
          <span className={styles.playerBackend}>{playbackBackend}</span>
        </div>

        <p className={styles.currentTrackTitle}>{currentTrackTitle}</p>

        <ProgressBar
          positionMs={playbackState.positionMs}
          durationMs={playbackState.durationMs}
          disabled={!playbackState.trackId || playbackState.durationMs <= 0}
          onSeek={onSeek}
        />
      </div>

      <div className={styles.controlsWrap}>
        <div
          className={`${styles.visualizer} ${styles[`state${effectiveVisualState[0].toUpperCase()}${effectiveVisualState.slice(1)}`]}`}
          aria-hidden="true"
        >
          <span className={styles.visualizerBar} />
          <span className={styles.visualizerBar} />
          <span className={styles.visualizerBar} />
          <span className={styles.visualizerBar} />
        </div>

        <div className={styles.playbackControls}>
          <button
            className={`${styles.secondaryButton} ${styles.transportButton} ${
              queueState.shuffleEnabled ? styles.isActive : ""
            }`}
            type="button"
            aria-pressed={queueState.shuffleEnabled}
            onClick={() => void onToggleShuffle()}
          >
            Shuffle
          </button>
          <button
            className={`${styles.secondaryButton} ${styles.transportButton} ${styles.repeatButton} ${
              queueState.repeatMode === "none" ? "" : styles.isActive
            } ${styles[`repeat${queueState.repeatMode[0].toUpperCase()}${queueState.repeatMode.slice(1)}`] ?? ""}`}
            type="button"
            aria-label={describeRepeatMode(queueState.repeatMode)}
            title={describeRepeatMode(queueState.repeatMode)}
            onClick={() => void onCycleRepeatMode()}
          >
            <span className={styles.repeatIcon} aria-hidden="true">
              <span className={styles.repeatLoop}>↻</span>
              <span className={styles.repeatBadge}>
                {queueState.repeatMode === "track"
                  ? "1"
                  : queueState.repeatMode === "playlist"
                    ? "•"
                    : ""}
              </span>
            </span>
          </button>
          <button
            className={`${styles.secondaryButton} ${styles.transportButton}`}
            type="button"
            disabled={transportDisabled || !queueState.canPlayPrevious}
            onClick={() => void onPlayPrevious()}
          >
            Prev
          </button>
          <button
            className={`${styles.primaryButton} ${styles.transportButton}`}
            type="button"
            disabled={
              playbackState.state === "idle" ||
              playbackState.state === "error" ||
              playbackState.state === "loading"
            }
            onClick={() => void onTogglePlayback()}
          >
            {playbackState.state === "paused" ? "Продолжить" : "Пауза"}
          </button>
          <button
            className={`${styles.secondaryButton} ${styles.transportButton}`}
            type="button"
            disabled={transportDisabled || !queueState.canPlayNext}
            onClick={() => void onPlayNext()}
          >
            Next
          </button>
        </div>

        <VolumeControl value={playbackState.volumePercent} onChange={onVolumeChange} />

        <OutputDeviceControl
          devices={audioOutputDevices}
          onRefresh={onRefreshOutputDevices}
          onSelect={onSelectOutputDevice}
        />

        <p className={styles.statusDetails}>{playbackStatusDetails}</p>
      </div>
    </section>
  );
}
