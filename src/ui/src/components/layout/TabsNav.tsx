import type { TabId } from "@models/models";
import styles from "./TabsNav.module.scss";

const tabs: Array<{ id: TabId; title: string }> = [
  { id: "home", title: "Главная" },
  { id: "search", title: "Поиск" },
  { id: "queue", title: "Очередь" },
  { id: "equalizer", title: "Эквалайзер" },
];

interface TabsNavProps {
  activeTabId: TabId;
  onSelect: (tabId: TabId) => void;
}

export function TabsNav({ activeTabId, onSelect }: TabsNavProps) {
  return (
    <nav className={styles.tabs} aria-label="Разделы приложения">
      {tabs.map((tab) => {
        const isActive = tab.id === activeTabId;
        return (
          <button
            key={tab.id}
            className={`${styles.tabButton} ${isActive ? styles.isActive : ""}`}
            type="button"
            aria-selected={isActive}
            onClick={() => onSelect(tab.id)}
          >
            {tab.title}
          </button>
        );
      })}
    </nav>
  );
}
