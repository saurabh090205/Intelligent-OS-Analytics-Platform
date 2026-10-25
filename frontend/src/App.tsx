import React, { useState, useEffect, useCallback, useRef } from 'react';
import { Sidebar } from './components/Sidebar';
import type { NavItemKey } from './components/Sidebar';
import { TopNavbar } from './components/TopNavbar';
import { ProcessModal } from './components/ProcessModal';
import { OverviewPage } from './pages/OverviewPage';
import { ProcessMonitorPage } from './pages/ProcessMonitorPage';
import { CpuSchedulingPage } from './pages/CpuSchedulingPage';
import { MemoryAnalyticsPage } from './pages/MemoryAnalyticsPage';
import { PageReplacementPage } from './pages/PageReplacementPage';
import { DeadlockPage } from './pages/DeadlockPage';
import { LogAnalysisPage } from './pages/LogAnalysisPage';
import { ResourceAdvisoryPage } from './pages/ResourceAdvisoryPage';
import { ModulePlaceholderPage } from './pages/ModulePlaceholderPage';
import { SettingsAboutPage } from './pages/SettingsAboutPage';
import { api } from './services/api';
import type { ApiHealth, SystemOverview, ProcessSummary, HistoryPoint } from './types/system';
import { AlertCircle, RefreshCw } from 'lucide-react';

export const App: React.FC = () => {
  const [currentTab, setCurrentTab] = useState<NavItemKey>('overview');
  const [health, setHealth] = useState<ApiHealth | null>(null);
  const [overview, setOverview] = useState<SystemOverview | null>(null);
  const [processes, setProcesses] = useState<ProcessSummary[]>([]);
  const [totalProcesses, setTotalProcesses] = useState<number>(0);
  const [history, setHistory] = useState<HistoryPoint[]>([]);
  const [refreshInterval, setRefreshInterval] = useState<number>(2);
  const [isRefreshing, setIsRefreshing] = useState<boolean>(false);
  const [isBackendConnected, setIsBackendConnected] = useState<boolean>(true);
  const [connectionError, setConnectionError] = useState<string | null>(null);
  const [inspectedPid, setInspectedPid] = useState<number | null>(null);
  const [searchQuery, setSearchQuery] = useState<string>('');

  const historyRef = useRef<HistoryPoint[]>([]);
  historyRef.current = history;

  const fetchData = useCallback(async () => {
    setIsRefreshing(true);
    try {
      const [overviewData, procsData, healthData] = await Promise.all([
        api.getOverview(),
        api.getProcesses(),
        api.getHealth(),
      ]);

      setOverview(overviewData);
      setProcesses(procsData.processes || []);
      setTotalProcesses(procsData.total || 0);
      setHealth(healthData);
      setIsBackendConnected(true);
      setConnectionError(null);

      // Add actual sample point to real-time session history
      const now = new Date();
      const timeStr = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
      const newPoint: HistoryPoint = {
        time: timeStr,
        timestamp: overviewData.timestamp,
        cpu: overviewData.cpu_utilization,
        memory: overviewData.memory_utilization,
      };

      setHistory((prev) => {
        const next = [...prev, newPoint];
        // Keep last 30 samples (e.g. 1-2 minutes of live data)
        return next.length > 30 ? next.slice(next.length - 30) : next;
      });
    } catch (err: unknown) {
      setIsBackendConnected(false);
      setConnectionError(err instanceof Error ? err.message : 'Backend connection failed');
    } finally {
      setIsRefreshing(false);
    }
  }, []);

  // Initial fetch and automatic polling interval
  useEffect(() => {
    fetchData();

    if (refreshInterval > 0) {
      const timer = setInterval(() => {
        fetchData();
      }, refreshInterval * 1000);
      return () => clearInterval(timer);
    }
  }, [fetchData, refreshInterval]);

  const cpuAlert = overview ? overview.cpu_utilization > 85 : false;
  const memoryAlert = overview ? overview.memory_utilization > 85 : false;

  return (
    <div className="flex h-screen bg-slate-50 overflow-hidden font-sans">
      {/* Sidebar Navigation */}
      <Sidebar
        currentTab={currentTab}
        onSelectTab={setCurrentTab}
        isBackendConnected={isBackendConnected}
        platformName={health?.platform}
      />

      {/* Main Content Area */}
      <div className="flex-1 flex flex-col min-w-0 overflow-hidden">
        {/* Top Navigation Bar */}
        <TopNavbar
          currentTab={currentTab}
          refreshInterval={refreshInterval}
          onIntervalChange={setRefreshInterval}
          onManualRefresh={fetchData}
          isRefreshing={isRefreshing}
          searchQuery={searchQuery}
          onSearchChange={setSearchQuery}
          cpuAlert={cpuAlert}
          memoryAlert={memoryAlert}
          uptimeSeconds={overview?.uptime_seconds}
        />

        {/* Backend Disconnection Banner */}
        {!isBackendConnected && (
          <div className="px-6 py-2.5 bg-rose-500 text-white text-xs font-medium flex items-center justify-between shrink-0 shadow-xs">
            <div className="flex items-center space-x-2">
              <AlertCircle className="w-4 h-4 shrink-0" />
              <span>
                <strong>Connection Error</strong>: Unable to communicate with C backend at <code>http://127.0.0.1:8080</code>. ({connectionError})
              </span>
            </div>
            <button
              onClick={fetchData}
              className="px-2.5 py-1 bg-white/20 hover:bg-white/30 text-white rounded text-xs transition-colors flex items-center space-x-1 cursor-pointer"
            >
              <RefreshCw className="w-3 h-3" />
              <span>Retry</span>
            </button>
          </div>
        )}

        {/* Scrollable View Container */}
        <main className="flex-1 overflow-y-auto p-6">
          {currentTab === 'overview' && (
            <OverviewPage
              overview={overview}
              history={history}
              processes={processes}
              onInspectProcess={(pid) => setInspectedPid(pid)}
              onNavigateToProcesses={() => setCurrentTab('processes')}
              onNavigateToTab={(tab) => setCurrentTab(tab)}
            />
          )}

          {currentTab === 'processes' && (
            <ProcessMonitorPage
              processes={processes}
              totalProcesses={totalProcesses}
              onInspectProcess={(pid) => setInspectedPid(pid)}
              onManualRefresh={fetchData}
              isRefreshing={isRefreshing}
              searchQuery={searchQuery}
              onSearchChange={setSearchQuery}
            />
          )}

          {currentTab === 'scheduling' && (
            <CpuSchedulingPage />
          )}

          {currentTab === 'memory' && (
            <MemoryAnalyticsPage />
          )}

          {currentTab === 'deadlock' && (
            <DeadlockPage />
          )}

          {currentTab === 'page-replacement' && (
            <PageReplacementPage />
          )}

          {currentTab === 'logs' && (
            <LogAnalysisPage />
          )}

          {currentTab === 'resource-allocation' && (
            <ResourceAdvisoryPage />
          )}

          {currentTab === 'settings' && (
            <SettingsAboutPage health={health} />
          )}

          {currentTab !== 'overview' &&
            currentTab !== 'processes' &&
            currentTab !== 'scheduling' &&
            currentTab !== 'memory' &&
            currentTab !== 'deadlock' &&
            currentTab !== 'page-replacement' &&
            currentTab !== 'logs' &&
            currentTab !== 'resource-allocation' &&
            currentTab !== 'settings' && (
            <ModulePlaceholderPage moduleKey={currentTab} />
          )}
        </main>
      </div>

      {/* Process Detail Inspector Modal */}
      <ProcessModal
        pid={inspectedPid}
        onClose={() => setInspectedPid(null)}
      />
    </div>
  );
};

export default App;
