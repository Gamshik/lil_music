import type { Track } from "@models/models";
import { TrackList } from "@components/tracks/TrackList";
import styles from "./TabSections.module.scss";

interface SearchTabProps {
  query: string;
  limit: number;
  offset: number;
  summary: string;
  tracks: Track[];
  currentTrackId: string;
  onQueryChange: (value: string) => void;
  onLimitChange: (value: number) => void;
  onOffsetChange: (value: number) => void;
  onSubmit: () => Promise<void>;
  onPlay: (track: Track) => Promise<void>;
  onQueue: (track: Track) => Promise<void>;
}

export function SearchTab({
  query,
  limit,
  offset,
  summary,
  tracks,
  currentTrackId,
  onQueryChange,
  onLimitChange,
  onOffsetChange,
  onSubmit,
  onPlay,
  onQueue,
}: SearchTabProps) {
  return (
    <section className={styles.panel}>
      <div className={styles.head}>
        <div>
          <p className={styles.kicker}>Поиск</p>
          <h1 className={styles.title}>Найди нужный трек</h1>
        </div>
      </div>

      <form
        className={styles.searchForm}
        onSubmit={(event) => {
          event.preventDefault();
          void onSubmit();
        }}
      >
        <label className={styles.searchField}>
          <span className={styles.fieldLabel}>Запрос</span>
          <input
            type="search"
            value={query}
            placeholder="lofi, ambient, techno, jungle..."
            autoComplete="off"
            onChange={(event) => onQueryChange(event.target.value)}
          />
        </label>

        <div className={styles.searchSide}>
          <label className={styles.parameterField}>
            <span className={styles.fieldLabel}>Лимит</span>
            <input
              type="number"
              min="1"
              max="50"
              value={limit}
              onChange={(event) => onLimitChange(Number(event.target.value))}
            />
          </label>

          <label className={styles.parameterField}>
            <span className={styles.fieldLabel}>Смещение</span>
            <input
              type="number"
              min="0"
              step="1"
              value={offset}
              onChange={(event) => onOffsetChange(Number(event.target.value))}
            />
          </label>

          <button className={styles.primaryButton} type="submit">
            Искать
          </button>
        </div>
      </form>

      <p className={styles.summary}>{summary}</p>

      <TrackList
        tracks={tracks}
        emptyMessage="По этому запросу SoundCloud не вернул совместимые треки."
        currentTrackId={currentTrackId}
        onPlay={onPlay}
        onQueue={onQueue}
      />
    </section>
  );
}
