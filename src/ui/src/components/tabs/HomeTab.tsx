import type { Track } from "@models/models";
import { TrackList } from "@components/tracks/TrackList";
import styles from "./TabSections.module.scss";

interface HomeTabProps {
  summary: string;
  tracks: Track[];
  currentTrackId: string;
  onRefresh: () => Promise<void>;
  onPlay: (track: Track) => Promise<void>;
  onQueue: (track: Track) => Promise<void>;
}

export function HomeTab({
  summary,
  tracks,
  currentTrackId,
  onRefresh,
  onPlay,
  onQueue,
}: HomeTabProps) {
  return (
    <section className={styles.panel}>
      <div className={styles.head}>
        <div>
          <p className={styles.kicker}>Главная</p>
          <h1 className={styles.title}>Популярное сейчас</h1>
        </div>
        <button className={styles.secondaryButton} type="button" onClick={() => void onRefresh()}>
          Обновить
        </button>
      </div>

      <p className={styles.summary}>{summary}</p>

      <TrackList
        tracks={tracks}
        emptyMessage="SoundCloud не вернул совместимые треки для стартовой страницы."
        currentTrackId={currentTrackId}
        onPlay={onPlay}
        onQueue={onQueue}
      />
    </section>
  );
}
