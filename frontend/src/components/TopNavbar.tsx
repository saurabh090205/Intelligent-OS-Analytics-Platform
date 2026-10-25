import React, { useState } from 'react';
import {
  RefreshCw,
  Search,
  Bell,
  Clock,
  CheckCircle2,
  AlertTriangle
} from 'lucide-react';
import type { NavItemKey } from './Sidebar';

interface TopNavbarProps {
  currentTab: NavItemKey;
  refreshInterval: number;
  onIntervalChange: (sec: number) => void;
  onManualRefresh: () => void;
  isRefreshing: boolean;
  searchQuery: string;
  onSearchChange: (q: string) => void;
  cpuAlert?: boolean;
  memoryAlert?: boolean;
  uptimeSeconds?: number;
}

const TAB_TITLES: Record<NavItemKey, { title: string; subtitle: string }> = {
  overview: { title: 'System Overview Dashboard', subtitle: 'Live CPU, memory, and process telemetry' },
  processes: { title: 'Process Monitoring', subtitle: 'Inspect active system processes from /proc' },
  scheduling: { title: 'CPU Scheduling Recommendation', subtitle: 'Phase 2: FCFS, SJF, SRTF, Priority, Round Robin' },
  memory: { title: 'Memory Usage Prediction', subtitle: 'Phase 2: Forecasting models and trend detection' },
  deadlock: { title: 'Deadlock Prediction & Detection', subtitle: 'Phase 3: Safe-state Banker\'s algorithm simulation' },
  'page-replacement': { title: 'Page Replacement Simulation', subtitle: 'Phase 2: FIFO, LRU, Optimal algorithms' },
  logs: { title: 'System Log Analysis', subtitle: 'Phase 3: Log parsing, severity counting, error detection' },
  'resource-allocation': { title: 'AI-Based Resource Allocation', subtitle: 'Phase 3: Explainable recommendations' },
  settings: { title: 'Platform Architecture & Settings', subtitle: 'Engine specs, C REST API endpoints, and diagnostics' },
};

function formatUptime(totalSec: number): string {
  if (!totalSec || totalSec <= 0) return '0s';
  const days = Math.floor(totalSec / 86400);
  const hours = Math.floor((totalSec % 86400) / 3600);
  const minutes = Math.floor((totalSec % 3600) / 60);
  const seconds = totalSec % 60;
  if (days > 0) return `${days}d ${hours}h ${minutes}m`;
  if (hours > 0) return `${hours}h ${minutes}m ${seconds}s`;
  return `${minutes}m ${seconds}s`;
}

export const TopNavbar: React.FC<TopNavbarProps> = ({
  currentTab,
  refreshInterval,
  onIntervalChange,
  onManualRefresh,
  isRefreshing,
  searchQuery,
  onSearchChange,
  cpuAlert,
  memoryAlert,
  uptimeSeconds = 0,
}) => {
  const [showNotifications, setShowNotifications] = useState(false);
  const tabInfo = TAB_TITLES[currentTab];
  const alertCount = (cpuAlert ? 1 : 0) + (memoryAlert ? 1 : 0);

  return (
    <header className="h-16 bg-white border-b border-slate-200 px-6 flex items-center justify-between shrink-0 select-none">
      {/* Title & Breadcrumbs */}
      <div>
        <div className="flex items-center space-x-2 text-xs text-slate-400">
          <span>Platform</span>
          <span>/</span>
          <span className="text-slate-600 font-medium capitalize">{currentTab.replace('-', ' ')}</span>
        </div>
        <h2 className="text-base font-bold text-slate-900 leading-tight">{tabInfo.title}</h2>
      </div>

      {/* Center/Right Controls */}
      <div className="flex items-center space-x-3">
        {/* Search bar (primarily for process monitoring or filtering) */}
        <div className="relative w-56">
          <Search className="w-3.5 h-3.5 absolute left-3 top-1/2 -translate-y-1/2 text-slate-400" />
          <input
            type="text"
            placeholder="Search processes, PID..."
            value={searchQuery}
            onChange={(e) => onSearchChange(e.target.value)}
            className="w-full bg-slate-50 border border-slate-200 rounded-md pl-8 pr-3 py-1.5 text-xs text-slate-800 placeholder-slate-400 focus:outline-hidden focus:ring-1 focus:ring-emerald-500 focus:border-emerald-500 transition-all"
          />
        </div>

        {/* Uptime Indicator */}
        <div className="hidden lg:flex items-center space-x-1.5 px-2.5 py-1.5 bg-slate-50 border border-slate-200 rounded-md text-xs text-slate-600">
          <Clock className="w-3.5 h-3.5 text-slate-400" />
          <span className="font-mono text-[11px]">{formatUptime(uptimeSeconds)}</span>
        </div>

        {/* Refresh Interval Selector */}
        <div className="flex items-center space-x-1.5 bg-slate-50 border border-slate-200 rounded-md px-2 py-1 text-xs">
          <span className="text-slate-400 text-[11px]">Auto:</span>
          <select
            value={refreshInterval}
            onChange={(e) => onIntervalChange(Number(e.target.value))}
            className="bg-transparent text-slate-700 text-xs font-medium cursor-pointer focus:outline-hidden"
          >
            <option value={1}>1s</option>
            <option value={2}>2s</option>
            <option value={5}>5s</option>
            <option value={10}>10s</option>
            <option value={0}>Pause</option>
          </select>
        </div>

        {/* Manual Refresh Button */}
        <button
          onClick={onManualRefresh}
          disabled={isRefreshing}
          className="flex items-center space-x-1.5 px-3 py-1.5 bg-emerald-600 hover:bg-emerald-700 text-white rounded-md text-xs font-medium transition-colors disabled:opacity-50 cursor-pointer shadow-2xs"
          title="Sample system metrics now"
        >
          <RefreshCw className={`w-3.5 h-3.5 ${isRefreshing ? 'animate-spin' : ''}`} />
          <span className="hidden sm:inline">{isRefreshing ? 'Sampling...' : 'Refresh'}</span>
        </button>

        {/* Notification Bell */}
        <div className="relative">
          <button
            onClick={() => setShowNotifications(!showNotifications)}
            className="relative p-2 rounded-md hover:bg-slate-100 text-slate-500 transition-colors cursor-pointer"
            title="System notifications"
          >
            <Bell className="w-4 h-4" />
            {alertCount > 0 && (
              <span className="absolute top-1.5 right-1.5 w-2 h-2 rounded-full bg-rose-500 ring-2 ring-white"></span>
            )}
          </button>

          {showNotifications && (
            <div className="absolute right-0 mt-2 w-72 bg-white rounded-lg shadow-lg border border-slate-200 p-3 z-50">
              <div className="flex items-center justify-between pb-2 border-b border-slate-100">
                <span className="text-xs font-semibold text-slate-700">System Telemetry Alerts</span>
                <span className="text-[10px] text-slate-400 font-mono">{alertCount} Active</span>
              </div>

              <div className="mt-2 space-y-2 text-xs">
                {cpuAlert && (
                  <div className="p-2 bg-amber-50 border border-amber-200 rounded text-amber-800 flex items-start space-x-2">
                    <AlertTriangle className="w-4 h-4 shrink-0 text-amber-600 mt-0.5" />
                    <div>
                      <p className="font-semibold text-[11px]">High CPU Utilization</p>
                      <p className="text-[10px] text-amber-700">CPU usage exceeded 85% threshold.</p>
                    </div>
                  </div>
                )}

                {memoryAlert && (
                  <div className="p-2 bg-rose-50 border border-rose-200 rounded text-rose-800 flex items-start space-x-2">
                    <AlertTriangle className="w-4 h-4 shrink-0 text-rose-600 mt-0.5" />
                    <div>
                      <p className="font-semibold text-[11px]">High Memory Pressure</p>
                      <p className="text-[10px] text-rose-700">Available physical RAM below 15% threshold.</p>
                    </div>
                  </div>
                )}

                {!cpuAlert && !memoryAlert && (
                  <div className="p-2 bg-emerald-50 border border-emerald-200 rounded text-emerald-800 flex items-center space-x-2">
                    <CheckCircle2 className="w-4 h-4 text-emerald-600 shrink-0" />
                    <span className="text-[11px]">System operating within normal resource bounds.</span>
                  </div>
                )}
              </div>
            </div>
          )}
        </div>
      </div>
    </header>
  );
};
