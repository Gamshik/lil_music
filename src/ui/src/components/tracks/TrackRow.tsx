import type { Track } from "@models/models";
import styles from "./TrackRow.module.scss";

interface TrackRowProps {
  track: Track;
  isCurrent: boolean;
  onPlay: (track: Track) => Promise<void>;
  onQueue: (track: Track) => Promise<void>;
}

function Artwork({ track }: { track: Track }) {
  if (!track.artworkUrl) {
    return <div className={styles.artworkFallback}>{(track.title || "?").trim().charAt(0).toUpperCase()}</div>;
  }

  return (
    <img
      className={styles.artwork}
      src={track.artworkUrl}
      alt={`Обложка ${track.title}`}
      loading="lazy"
      referrerPolicy="no-referrer"
    />
  );
}

export function TrackRow({ track, isCurrent, onPlay, onQueue }: TrackRowProps) {
  return (
    <li className={`${styles.trackRow} ${isCurrent ? styles.isCurrent : ""}`}>
      <div className={styles.artworkFrame}>
        <Artwork track={track} />
      </div>

      <div className={styles.main}>
        <div className={styles.topline}>
          <p className={styles.title}>{track.title}</p>
          <p className={styles.id}>{track.id}</p>
        </div>
        <p className={styles.artist}>{track.artistName || "Неизвестный артист"}</p>
      </div>

      <div className={styles.actions}>
        <button className={styles.queueButton} type="button" onClick={() => void onQueue(track)}>
          В очередь
        </button>
        <button className={styles.playButton} type="button" onClick={() => void onPlay(track)}>
          Играть
        </button>
      </div>
    </li>
  );
}
