import { useEffect, useRef, useState } from "react";
import type { EqualizerBand, EqualizerPresetId, EqualizerState } from "@models/models";
import { clampEqualizerGainDb } from "@utils/math";
import {
  formatEqualizerGainDb,
  formatEqualizerFrequencyLabel,
  getEqualizerPresetTitle,
} from "@utils/formatters";
import { EqualizerSlider } from "./EqualizerSlider";
import styles from "./EqualizerPanel.module.scss";

interface EqualizerPanelProps {
  equalizerState: EqualizerState;
  statusLabel: string;
  summary: string;
  onToggleEnabled: (enabled: boolean) => Promise<void>;
  onSelectPreset: (presetId: EqualizerPresetId) => Promise<void>;
  onSetBandGain: (bandIndex: number, gainDb: number) => Promise<void>;
  onSetOutputGain: (gainDb: number) => Promise<void>;
  onReset: () => Promise<void>;
}

function derivePresetIdFromBands(equalizerState: EqualizerState, bands: EqualizerBand[]): EqualizerPresetId {
  const matchingPreset = equalizerState.presets.find((preset) => {
    if (preset.id === "custom") {
      return false;
    }

    return preset.gainsDb.every((gainDb, index) => {
      const band = bands[index];
      return band && Math.abs((band.gainDb || 0) - (gainDb || 0)) < 0.01;
    });
  });

  return matchingPreset?.id || "custom";
}

export function EqualizerPanel({
  equalizerState,
  statusLabel,
  summary,
  onToggleEnabled,
  onSelectPreset,
  onSetBandGain,
  onSetOutputGain,
  onReset,
}: EqualizerPanelProps) {
  const [draftBands, setDraftBands] = useState(equalizerState.bands);
  const [draftOutputGain, setDraftOutputGain] = useState(equalizerState.outputGainDb);
  const bandTimersRef = useRef(new Map<number, number>());
  const outputGainTimerRef = useRef<number | null>(null);

  useEffect(() => {
    setDraftBands(equalizerState.bands);
    setDraftOutputGain(equalizerState.outputGainDb);
  }, [equalizerState.bands, equalizerState.outputGainDb]);

  const controlsEnabled = equalizerState.status === "ready";
  const activePresetId = derivePresetIdFromBands(equalizerState, draftBands);

  const clearPendingSync = () => {
    bandTimersRef.current.forEach((timerId) => {
      window.clearTimeout(timerId);
    });
    bandTimersRef.current.clear();

    if (outputGainTimerRef.current !== null) {
      window.clearTimeout(outputGainTimerRef.current);
      outputGainTimerRef.current = null;
    }
  };

  useEffect(() => clearPendingSync, []);

  const scheduleBandSync = (bandIndex: number, gainDb: number) => {
    const existingTimer = bandTimersRef.current.get(bandIndex);
    if (existingTimer) {
      window.clearTimeout(existingTimer);
    }

    const timerId = window.setTimeout(() => {
      bandTimersRef.current.delete(bandIndex);
      void onSetBandGain(bandIndex, gainDb);
    }, 70);

    bandTimersRef.current.set(bandIndex, timerId);
  };

  const scheduleOutputGainSync = (gainDb: number) => {
    if (outputGainTimerRef.current !== null) {
      window.clearTimeout(outputGainTimerRef.current);
    }

    outputGainTimerRef.current = window.setTimeout(() => {
      outputGainTimerRef.current = null;
      void onSetOutputGain(gainDb);
    }, 70);
  };

  const flushBandSync = async (bandIndex: number, gainDb: number) => {
    const existingTimer = bandTimersRef.current.get(bandIndex);
    if (existingTimer) {
      window.clearTimeout(existingTimer);
      bandTimersRef.current.delete(bandIndex);
    }

    await onSetBandGain(bandIndex, gainDb);
  };

  const flushOutputGain = async (gainDb: number) => {
    if (outputGainTimerRef.current !== null) {
      window.clearTimeout(outputGainTimerRef.current);
      outputGainTimerRef.current = null;
    }

    await onSetOutputGain(gainDb);
  };

  return (
    <section className={styles.equalizerShell}>
      <div className={styles.sectionHead}>
        <div>
          <p className={styles.sectionKicker}>Эквалайзер</p>
          <h1 className={styles.title}>Настрой звук под себя</h1>
        </div>

        <div className={styles.toolbar}>
          <span className={styles.statusPill}>{statusLabel}</span>
          <button className={styles.secondaryButton} type="button" onClick={() => void onReset()}>
            Reset
          </button>
        </div>
      </div>

      <p className={styles.summary}>{summary}</p>

      <section className={styles.surface}>
        <div className={styles.topline}>
          <div className={styles.copy}>
            <p className={styles.fieldLabel}>Master EQ</p>
            <strong className={styles.presetTitle}>
              {getEqualizerPresetTitle(equalizerState.presets, activePresetId)}
            </strong>
            <p className={styles.meta}>
              Level: {formatEqualizerGainDb(draftOutputGain)} · Auto headroom:{" "}
              {formatEqualizerGainDb(equalizerState.headroomCompensationDb)}
            </p>
          </div>

          <div className={styles.master}>
            <span className={styles.statusDetail}>
              {equalizerState.errorMessage || "DSP активен в native audio engine."}
            </span>
          </div>
        </div>

        <div className={styles.mainToggle}>
          <button
            className={`${styles.toggleButton} ${equalizerState.enabled ? styles.isEnabled : ""}`}
            type="button"
            aria-pressed={equalizerState.enabled}
            disabled={!controlsEnabled}
            onClick={() => {
              clearPendingSync();
              void onToggleEnabled(!equalizerState.enabled);
            }}
          >
            <span className={styles.toggleTrack}>
              <span className={styles.toggleThumb} />
            </span>
            <span className={styles.toggleCopy}>Включить EQ</span>
          </button>
        </div>

        <div className={styles.presets}>
          {equalizerState.presets.map((preset) => {
            const isActive = preset.id === activePresetId;
            return (
              <button
                key={preset.id}
                className={`${styles.presetButton} ${isActive ? styles.isActive : ""} ${
                  preset.id === "custom" ? styles.isDerived : ""
                }`}
                type="button"
                disabled={!controlsEnabled || preset.id === "custom"}
                aria-pressed={isActive}
                onClick={() => {
                  clearPendingSync();
                  void onSelectPreset(preset.id);
                }}
              >
                {preset.title}
              </button>
            );
          })}
        </div>

        <div className={styles.bands} aria-label="Ручная настройка полос эквалайзера">
          <EqualizerSlider
            label="level"
            valueDb={draftOutputGain}
            disabled={!controlsEnabled}
            output
            onPreview={(gainDb) => {
              const safeGainDb = clampEqualizerGainDb(gainDb);
              setDraftOutputGain(safeGainDb);
              scheduleOutputGainSync(safeGainDb);
            }}
            onCommit={flushOutputGain}
          />

          {draftBands.map((band) => (
            <EqualizerSlider
              key={band.index}
              label={formatEqualizerFrequencyLabel(band.centerFrequencyHz)}
              valueDb={band.gainDb}
              disabled={!controlsEnabled}
              onPreview={(gainDb) => {
                const safeGainDb = clampEqualizerGainDb(gainDb);
                setDraftBands((currentBands) =>
                  currentBands.map((currentBand) =>
                    currentBand.index === band.index
                      ? { ...currentBand, gainDb: safeGainDb }
                      : currentBand,
                  ),
                );
                scheduleBandSync(band.index, safeGainDb);
              }}
              onCommit={(gainDb) => flushBandSync(band.index, gainDb)}
            />
          ))}
        </div>
      </section>
    </section>
  );
}
