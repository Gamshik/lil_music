import { useDeferredValue } from "react";
import type { Track } from "@models/models";
import { TrackRow } from "./TrackRow";
import styles from "./TrackList.module.scss";

interface TrackListProps {
  tracks: Track[];
  emptyMessage: string;
  currentTrackId: string;
  onPlay: (track: Track) => Promise<void>;
  onQueue: (track: Track) => Promise<void>;
}

export function TrackList({
  tracks,
  emptyMessage,
  currentTrackId,
  onPlay,
  onQueue,
}: TrackListProps) {
  const deferredTracks = useDeferredValue(tracks);

  return (
    <ul className={styles.trackList}>
      {deferredTracks.length === 0 ? (
        <li className={styles.emptyState}>{emptyMessage}</li>
      ) : (
        deferredTracks.map((track) => (
          <TrackRow
            key={track.id}
            track={track}
            isCurrent={track.id === currentTrackId}
            onPlay={onPlay}
            onQueue={onQueue}
          />
        ))
      )}
    </ul>
  );
}
