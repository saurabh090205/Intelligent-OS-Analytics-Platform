import React from 'react';
import {
  Activity,
  Cpu,
  Layers,
  Database,
  ShieldAlert,
  HardDrive,
  FileText,
  Sliders,
  Settings,
  Terminal,
  CircleDot
} from 'lucide-react';

export type NavItemKey =
  | 'overview'
  | 'processes'
  | 'scheduling'
  | 'memory'
  | 'deadlock'
  | 'page-replacement'
  | 'logs'
  | 'resource-allocation'
  | 'settings';

interface SidebarProps {
  currentTab: NavItemKey;
  onSelectTab: (tab: NavItemKey) => void;
  isBackendConnected: boolean;
  platformName?: string;
}

interface NavItemDef {
  key: NavItemKey;
  label: string;
  icon: React.ComponentType<{ className?: string }>;
  phase: string;
  isImplemented: boolean;
}

const NAV_ITEMS: NavItemDef[] = [
  { key: 'overview', label: 'Overview Dashboard', icon: Activity, phase: 'Phase 1', isImplemented: true },
  { key: 'processes', label: 'Process Monitoring', icon: Terminal, phase: 'Phase 1', isImplemented: true },
  { key: 'scheduling', label: 'CPU Scheduling', icon: Cpu, phase: 'Phase 2', isImplemented: true },
  { key: 'memory', label: 'Memory Analytics', icon: Database, phase: 'Phase 2', isImplemented: true },
  { key: 'deadlock', label: 'Deadlock Detection', icon: ShieldAlert, phase: 'Phase 3', isImplemented: true },
  { key: 'page-replacement', label: 'Page Replacement', icon: Layers, phase: 'Phase 2', isImplemented: true },
  { key: 'logs', label: 'System Log Analysis', icon: FileText, phase: 'Phase 3', isImplemented: true },
  { key: 'resource-allocation', label: 'Resource Allocation', icon: Sliders, phase: 'Phase 3', isImplemented: true },
  { key: 'settings', label: 'Settings & Architecture', icon: Settings, phase: 'Phase 1', isImplemented: true },
];

export const Sidebar: React.FC<SidebarProps> = ({
  currentTab,
  onSelectTab,
  isBackendConnected,
  platformName,
}) => {
  return (
    <aside className="w-64 bg-slate-900 text-slate-300 flex flex-col shrink-0 h-screen border-r border-slate-800 select-none">
      {/* Brand Header */}
      <div className="p-4 border-b border-slate-800">
        <div className="flex items-center space-x-3">
          <div className="w-9 h-9 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center text-emerald-400">
            <HardDrive className="w-5 h-5" />
          </div>
          <div>
            <h1 className="text-sm font-bold text-white tracking-tight">OS Analytics Platform</h1>
            <p className="text-[11px] text-emerald-400/90 font-medium">Intelligent Resource Manager</p>
          </div>
        </div>
      </div>

      {/* Navigation List */}
      <nav className="flex-1 overflow-y-auto px-3 py-4 space-y-1">
        <div className="px-2 pb-2 text-[10px] font-semibold tracking-wider text-slate-500 uppercase">
          System Modules
        </div>

        {NAV_ITEMS.map((item) => {
          const Icon = item.icon;
          const isActive = currentTab === item.key;

          return (
            <button
              key={item.key}
              onClick={() => onSelectTab(item.key)}
              className={`w-full flex items-center justify-between px-3 py-2 text-xs font-medium rounded-md transition-colors ${
                isActive
                  ? 'bg-emerald-600 text-white shadow-xs'
                  : 'text-slate-400 hover:text-slate-100 hover:bg-slate-800/60'
              }`}
            >
              <div className="flex items-center space-x-3 truncate">
                <Icon className={`w-4 h-4 shrink-0 ${isActive ? 'text-white' : 'text-slate-400'}`} />
                <span className="truncate">{item.label}</span>
              </div>

              <span
                className={`text-[9px] px-1.5 py-0.5 rounded font-mono ${
                  isActive
                    ? 'bg-emerald-700 text-emerald-100'
                    : item.isImplemented
                    ? 'bg-emerald-950 text-emerald-400 border border-emerald-800/40'
                    : 'bg-slate-800 text-slate-400'
                }`}
              >
                {item.phase}
              </span>
            </button>
          );
        })}
      </nav>

      {/* Backend Health Status Footer */}
      <div className="p-3 border-t border-slate-800 bg-slate-950/60">
        <div className="flex items-center justify-between text-xs">
          <div className="flex items-center space-x-2">
            <span className="relative flex h-2 w-2">
              {isBackendConnected && (
                <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>
              )}
              <span
                className={`relative inline-flex rounded-full h-2 w-2 ${
                  isBackendConnected ? 'bg-emerald-500' : 'bg-rose-500'
                }`}
              ></span>
            </span>
            <span className="text-[11px] font-mono text-slate-300">
              {isBackendConnected ? 'C Backend Online' : 'Backend Offline'}
            </span>
          </div>
          <span className="text-[10px] text-slate-500 font-mono">Port 8080</span>
        </div>

        {platformName && (
          <div className="mt-1.5 text-[10px] text-slate-400 truncate flex items-center space-x-1">
            <CircleDot className="w-2.5 h-2.5 text-slate-500" />
            <span className="truncate">{platformName}</span>
          </div>
        )}
      </div>
    </aside>
  );
};
