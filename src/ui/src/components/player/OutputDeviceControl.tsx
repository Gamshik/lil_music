import type { AudioOutputDevice } from "@models/models";
import styles from "./OutputDeviceControl.module.scss";

interface OutputDeviceControlProps {
  devices: AudioOutputDevice[];
  onRefresh: () => Promise<void>;
  onSelect: (deviceId: string) => Promise<void>;
}

export function OutputDeviceControl({
  devices,
  onRefresh,
  onSelect,
}: OutputDeviceControlProps) {
  const selectedDevice = devices.find((device) => device.isSelected) ?? devices[0];
  const summary = selectedDevice
    ? selectedDevice.isDefault
      ? `${selectedDevice.displayName} · по умолчанию`
      : selectedDevice.displayName
    : "Нет активных устройств";

  return (
    <div className={styles.control}>
      <div className={styles.head}>
        <label className={styles.label} htmlFor="audio-output-device-select">
          Вывод
        </label>
        <span className={styles.summary}>{summary}</span>
      </div>

      <div className={styles.row}>
        <div className={styles.selectWrap}>
          <select
            id="audio-output-device-select"
            className={styles.select}
            aria-label="Устройство вывода аудио"
            value={selectedDevice?.id ?? ""}
            disabled={devices.length === 0}
            onChange={(event) => {
              void onSelect(event.target.value);
            }}
          >
            {devices.length === 0 ? (
              <option value="">Устройства вывода недоступны</option>
            ) : (
              devices.map((device) => (
                <option key={device.id} value={device.id}>
                  {device.isDefault ? `${device.displayName} · по умолчанию` : device.displayName}
                </option>
              ))
            )}
          </select>
          <span className={styles.chevron} aria-hidden="true" />
        </div>

        <button className={styles.refreshButton} type="button" onClick={() => void onRefresh()}>
          Обновить
        </button>
      </div>
    </div>
  );
}
