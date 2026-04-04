import type { Track } from "@models/models";
import { TopBar } from "@components/layout/TopBar";
import { TabsNav } from "@components/layout/TabsNav";
import { PlayerShell } from "@components/player/PlayerShell";
import { HomeTab } from "@components/tabs/HomeTab";
import { SearchTab } from "@components/tabs/SearchTab";
import { QueueTab } from "@components/tabs/QueueTab";
import { EqualizerTab } from "@components/tabs/EqualizerTab";
import { useLilMusicApp } from "@app/useLilMusicApp";
import styles from "./App.module.scss";

export function App() {
  const app = useLilMusicApp();

  const runGuarded = async (label: string, action: () => Promise<void>) => {
    try {
      await action();
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      app.reportStatusError(label, errorMessage);
    }
  };

  const handlePlayTrack = async (track: Track) => {
    await runGuarded("Ошибка playback", () => app.playTrack(track));
  };

  const handleQueueTrack = async (track: Track) => {
    await runGuarded("Ошибка очереди", () => app.enqueueTrack(track));
  };

  return (
    <>
      <div className={`${styles.ambient} ${styles.ambientCyan}`} aria-hidden="true" />
      <div className={`${styles.ambient} ${styles.ambientAmber}`} aria-hidden="true" />

      <main className={styles.appShell}>
        <TopBar bridgeTitle={app.bridgeTitle} bridgeDetails={app.bridgeDetails} />

        <TabsNav activeTabId={app.activeTabId} onSelect={app.setTab} />

        <PlayerShell
          playbackBackend={app.playbackBackend}
          playbackState={app.playbackState}
          playbackStatusLabel={app.playbackStatusView.label}
          playbackStatusDetails={app.playbackStatusView.details}
          currentTrackTitle={app.currentTrackTitle}
          queueState={app.queueState}
          audioOutputDevices={app.audioOutputDevices}
          isTransportBusy={app.isTransportBusy}
          onTogglePlayback={() => runGuarded("Ошибка playback", app.togglePlayback)}
          onPlayPrevious={() => runGuarded("Ошибка playback", app.playPreviousTrack)}
          onPlayNext={() => runGuarded("Ошибка playback", app.playNextTrack)}
          onSeek={(positionMs) => runGuarded("Ошибка playback", () => app.seekPlayback(positionMs))}
          onVolumeChange={(volumePercent) =>
            runGuarded("Ошибка playback", () => app.setPlaybackVolume(volumePercent))
          }
          onRefreshOutputDevices={() =>
            runGuarded("Ошибка вывода", app.refreshAudioOutputDevices)
          }
          onSelectOutputDevice={(deviceId) =>
            runGuarded("Ошибка вывода", () => app.selectAudioOutputDevice(deviceId))
          }
          onToggleShuffle={() => runGuarded("Ошибка playback", app.toggleShuffle)}
          onCycleRepeatMode={() => runGuarded("Ошибка playback", app.cycleRepeatMode)}
        />

        <section className={styles.contentShell}>
          {app.activeTabId === "home" && (
            <HomeTab
              summary={app.featuredSummary}
              tracks={app.featuredTracks}
              currentTrackId={app.currentTrackId}
              onRefresh={app.loadFeaturedTracks}
              onPlay={handlePlayTrack}
              onQueue={handleQueueTrack}
            />
          )}

          {app.activeTabId === "search" && (
            <SearchTab
              query={app.searchQuery}
              limit={app.searchLimit}
              offset={app.searchOffset}
              summary={app.searchSummary}
              tracks={app.searchTracks}
              currentTrackId={app.currentTrackId}
              onQueryChange={app.setSearchQuery}
              onLimitChange={app.setSearchLimit}
              onOffsetChange={app.setSearchOffset}
              onSubmit={app.submitSearch}
              onPlay={handlePlayTrack}
              onQueue={handleQueueTrack}
            />
          )}

          {app.activeTabId === "queue" && (
            <QueueTab
              queueState={app.queueState}
              onRemove={(trackId) => runGuarded("Ошибка очереди", () => app.removeQueuedTrack(trackId))}
            />
          )}

          {app.activeTabId === "equalizer" && (
            <EqualizerTab
              equalizerState={app.equalizerState}
              statusLabel={app.equalizerStatusLabel}
              summary={app.equalizerStatusSummary}
              onToggleEnabled={(enabled) =>
                runGuarded("Ошибка EQ", () => app.setEqualizerEnabled(enabled))
              }
              onSelectPreset={(presetId) =>
                runGuarded("Ошибка EQ", () => app.selectEqualizerPreset(presetId))
              }
              onSetBandGain={(bandIndex, gainDb) =>
                runGuarded("Ошибка EQ", () => app.setEqualizerBandGain(bandIndex, gainDb))
              }
              onSetOutputGain={(gainDb) =>
                runGuarded("Ошибка EQ", () => app.setEqualizerOutputGain(gainDb))
              }
              onReset={() => runGuarded("Ошибка EQ", app.resetEqualizer)}
            />
          )}
        </section>
      </main>
    </>
  );
}
