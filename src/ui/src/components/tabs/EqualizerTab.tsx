import type { EqualizerPresetId, EqualizerState } from "@models/models";
import { EqualizerPanel } from "@components/equalizer/EqualizerPanel";

interface EqualizerTabProps {
  equalizerState: EqualizerState;
  statusLabel: string;
  summary: string;
  onToggleEnabled: (enabled: boolean) => Promise<void>;
  onSelectPreset: (presetId: EqualizerPresetId) => Promise<void>;
  onSetBandGain: (bandIndex: number, gainDb: number) => Promise<void>;
  onSetOutputGain: (gainDb: number) => Promise<void>;
  onReset: () => Promise<void>;
}

export function EqualizerTab(props: EqualizerTabProps) {
  return <EqualizerPanel {...props} />;
}
