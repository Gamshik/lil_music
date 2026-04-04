import type { QueueState } from "@models/models";
import styles from "./QueueList.module.scss";

interface QueueListProps {
  queueState: QueueState;
  onRemove: (trackId: string) => Promise<void>;
}

export function QueueList({ queueState, onRemove }: QueueListProps) {
  return (
    <ul className={styles.queueList}>
      {queueState.queuedTracks.length === 0 ? (
        <li className={styles.emptyState}>Следующим будет трек по текущему списку.</li>
      ) : (
        queueState.queuedTracks.map((track) => (
          <li
            key={track.id}
            className={`${styles.queueItem} ${
              queueState.currentTrackId === track.id ? styles.isCurrent : ""
            }`}
          >
            <div className={styles.meta}>
              <span className={styles.title}>{track.title}</span>
              <span className={styles.artist}>{track.artistName || "Неизвестный артист"}</span>
            </div>
            <button className={styles.removeButton} type="button" onClick={() => void onRemove(track.id)}>
              Убрать
            </button>
          </li>
        ))
      )}
    </ul>
  );
}
