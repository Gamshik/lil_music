import type { QueueState } from "@models/models";
import { QueueList } from "@components/queue/QueueList";
import styles from "./TabSections.module.scss";

interface QueueTabProps {
  queueState: QueueState;
  onRemove: (trackId: string) => Promise<void>;
}

export function QueueTab({ queueState, onRemove }: QueueTabProps) {
  const queueCountText =
    queueState.queuedTracks.length === 0
      ? "Очередь пуста"
      : `${queueState.queuedTracks.length} в очереди`;

  return (
    <section className={styles.panel}>
      <div className={styles.head}>
        <div>
          <p className={styles.kicker}>Очередь</p>
          <h1 className={styles.title}>Что будет дальше</h1>
        </div>
        <span className={styles.queueCount}>{queueCountText}</span>
      </div>

      <p className={styles.summary}>
        Добавленные вручную треки идут первыми. Если очередь пустая, включается следующий трек из
        текущего списка.
      </p>

      <QueueList queueState={queueState} onRemove={onRemove} />
    </section>
  );
}
