import styles from "./TopBar.module.scss";

interface TopBarProps {
  bridgeTitle: string;
  bridgeDetails: string;
}

export function TopBar({ bridgeTitle, bridgeDetails }: TopBarProps) {
  return (
    <header className={styles.topbar}>
      <div className={styles.brandGroup}>
        <span className={styles.brandMark}>LM</span>
        <div className={styles.brandCopy}>
          <strong className={styles.brandTitle}>LilMusic</strong>
          <span className={styles.brandSubtitle}>тихий desktop-плеер для SoundCloud</span>
        </div>
      </div>

      <div className={styles.status}>
        <strong className={styles.statusTitle}>{bridgeTitle}</strong>
        <span className={styles.statusDetails}>{bridgeDetails}</span>
      </div>
    </header>
  );
}
