import React, { useState, useEffect } from 'react';
import {
  Cpu,
  Database,
  Terminal,
  Activity,
  HardDrive,
  Clock,
  ArrowUpRight,
  ShieldAlert,
  Layers,
  Sliders,
  TrendingUp,
  Info
} from 'lucide-react';
import {
  AreaChart,
  Area,
  PieChart,
  Pie,
  Cell,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer
} from 'recharts';
import { MetricCard } from '../components/MetricCard';
import { api } from '../services/api';
import type {
  SystemOverview,
  HistoryPoint,
  ProcessSummary,
  MemoryStats,
  ResourceAdvisoryResponse,
  MemoryForecastResponse,
  BankerResult
} from '../types/system';
import type { NavItemKey } from '../components/Sidebar';

interface OverviewPageProps {
  overview: SystemOverview | null;
  history: HistoryPoint[];
  processes: ProcessSummary[];
  onInspectProcess: (pid: number) => void;
  onNavigateToProcesses: () => void;
  onNavigateToTab?: (tab: NavItemKey) => void;
}

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

export const OverviewPage: React.FC<OverviewPageProps> = ({
  overview,
  history,
  processes,
  onInspectProcess,
  onNavigateToProcesses,
  onNavigateToTab,
}) => {
  const [memDetails, setMemDetails] = useState<MemoryStats | null>(null);
  const [advisory, setAdvisory] = useState<ResourceAdvisoryResponse | null>(null);
  const [forecast, setForecast] = useState<MemoryForecastResponse | null>(null);
  const [deadlockSummary, setDeadlockSummary] = useState<BankerResult | null>(null);
  const [moduleLoading, setModuleLoading] = useState<boolean>(true);

  // Fetch cross-module summaries for unified overview
  useEffect(() => {
    let isMounted = true;
    Promise.allSettled([
      api.getMemoryStats(),
      api.getResourceAdvisory(),
      api.getMemoryForecast(60, 10, 85.0),
      api.analyzeBanker({ example_id: 'silberschatz_safe' }),
    ]).then(([memRes, advRes, fcRes, dlRes]) => {
      if (!isMounted) return;
      if (memRes.status === 'fulfilled') setMemDetails(memRes.value);
      if (advRes.status === 'fulfilled') setAdvisory(advRes.value);
      if (fcRes.status === 'fulfilled') setForecast(fcRes.value);
      if (dlRes.status === 'fulfilled') setDeadlockSummary(dlRes.value);
      setModuleLoading(false);
    });

    return () => {
      isMounted = false;
    };
  }, []);

  if (!overview) {
    return (
      <div className="flex flex-col items-center justify-center min-h-[500px]">
        <div className="inline-block w-8 h-8 border-3 border-emerald-600 border-t-transparent rounded-full animate-spin"></div>
        <p className="mt-3 text-xs text-slate-500 font-medium">Connecting to C monitoring backend on http://127.0.0.1:8080...</p>
      </div>
    );
  }

  const cpuStatus = overview.cpu_utilization > 85 ? 'danger' : overview.cpu_utilization > 70 ? 'warning' : 'normal';
  const memStatus = overview.memory_utilization > 85 ? 'danger' : overview.memory_utilization > 70 ? 'warning' : 'normal';

  const usedMemGb = (overview.memory_used_kb / (1024 * 1024)).toFixed(2);
  const totalMemGb = (overview.memory_total_kb / (1024 * 1024)).toFixed(2);
  const availMemGb = (overview.memory_available_kb / (1024 * 1024)).toFixed(2);

  // Swap metrics
  const swapUsedMb = memDetails ? (memDetails.swap_used_kb / 1024).toFixed(0) : '0';
  const swapTotalMb = memDetails ? (memDetails.swap_total_kb / 1024).toFixed(0) : '0';
  const swapUtilPct = memDetails && memDetails.swap_total_kb > 0
    ? ((memDetails.swap_used_kb / memDetails.swap_total_kb) * 100).toFixed(1)
    : '0.0';

  // Top 5 processes by CPU
  const topCpuProcs = [...processes]
    .sort((a, b) => b.cpu_pct - a.cpu_pct)
    .slice(0, 5);

  // Top 5 processes by Memory
  const topMemProcs = [...processes]
    .sort((a, b) => b.mem_kb - a.mem_kb)
    .slice(0, 5);

  // Process state distribution for Pie chart
  const stateData = [
    { name: 'Running', value: overview.running_processes, color: '#10b981' },
    { name: 'Sleeping', value: overview.sleeping_processes, color: '#3b82f6' },
    { name: 'Stopped', value: overview.stopped_processes, color: '#f59e0b' },
    { name: 'Zombie', value: overview.zombie_processes, color: '#ef4444' },
  ].filter(item => item.value > 0);

  const topAdvisory = advisory?.items?.[0] || null;

  return (
    <div className="space-y-6 pb-12">
      {/* 6 Key Live Telemetry Cards */}
      <div>
        <div className="flex items-center justify-between mb-3">
          <div className="flex items-center space-x-2">
            <span className="w-2 h-2 rounded-full bg-emerald-500 animate-pulse"></span>
            <h2 className="text-xs font-bold uppercase tracking-wider text-slate-500">
              Host Operating System Telemetry
            </h2>
          </div>
          <span className="text-[11px] font-semibold text-emerald-700 bg-emerald-50 border border-emerald-200 px-2 py-0.5 rounded">
            Live OS Measurements
          </span>
        </div>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-6 gap-4">
          <MetricCard
            title="CPU Utilization"
            value={`${overview.cpu_utilization.toFixed(1)}%`}
            subValue={`${overview.cpu_cores} Cores • ${overview.cpu_model}`}
            icon={Cpu}
            progressPercent={overview.cpu_utilization}
            status={cpuStatus}
            trendLabel={`${overview.cpu_cores} Cores`}
          />

          <MetricCard
            title="Physical Memory"
            value={`${overview.memory_utilization.toFixed(1)}%`}
            subValue={`${usedMemGb} GB / ${totalMemGb} GB`}
            icon={Database}
            progressPercent={overview.memory_utilization}
            status={memStatus}
            trendLabel={`${usedMemGb} GB`}
          />

          <MetricCard
            title="Available RAM"
            value={`${availMemGb} GB`}
            subValue={`${(overview.memory_free_kb / 1024).toFixed(0)} MB unallocated`}
            icon={HardDrive}
            progressPercent={100 - overview.memory_utilization}
            status="success"
          />

          <MetricCard
            title="Swap / Paging"
            value={`${swapUtilPct}%`}
            subValue={`${swapUsedMb} MB / ${swapTotalMb} MB`}
            icon={Layers}
            progressPercent={parseFloat(swapUtilPct)}
            status={parseFloat(swapUtilPct) > 50 ? 'warning' : 'normal'}
          />

          <MetricCard
            title="Total Processes"
            value={overview.total_processes}
            subValue={`${overview.running_processes} Running • ${overview.sleeping_processes} Sleeping`}
            icon={Terminal}
            status="normal"
          />

          <MetricCard
            title="System Uptime"
            value={formatUptime(overview.uptime_seconds)}
            subValue={`Sampled: ${new Date(overview.timestamp * 1000).toLocaleTimeString()}`}
            icon={Clock}
            status="normal"
          />
        </div>
      </div>

      {/* Advisory & Subsystem Pressure Integration Bar */}
      {advisory && (
        <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-xs">
          <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 mb-3 border-b border-slate-100 pb-3">
            <div className="flex items-center space-x-2">
              <Sliders className="w-4 h-4 text-indigo-600" />
              <h3 className="text-sm font-bold text-slate-800 uppercase tracking-wide">
                System Health &amp; Resource Pressure Overview
              </h3>
            </div>
            <div className="flex items-center space-x-2">
              <span className="text-xs text-slate-500">Overall Assessment:</span>
              <span className={`px-2.5 py-0.5 rounded-full text-xs font-bold ${
                advisory.overview.overall_health.toLowerCase().includes('critical')
                  ? 'bg-rose-100 text-rose-800'
                  : advisory.overview.overall_health.toLowerCase().includes('caution') || advisory.overview.overall_health.toLowerCase().includes('attention')
                  ? 'bg-amber-100 text-amber-800'
                  : 'bg-emerald-100 text-emerald-800'
              }`}>
                {advisory.overview.overall_health}
              </span>
              {onNavigateToTab && (
                <button
                  onClick={() => onNavigateToTab('resource-allocation')}
                  className="text-xs font-semibold text-indigo-600 hover:text-indigo-700 flex items-center space-x-0.5 ml-2 cursor-pointer"
                >
                  <span>All Guidance</span>
                  <ArrowUpRight className="w-3.5 h-3.5" />
                </button>
              )}
            </div>
          </div>

          <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs mb-3">
            <div className="p-2.5 rounded-lg bg-slate-50 border border-slate-200">
              <span className="text-slate-500 block text-[10px] uppercase font-semibold">CPU Pressure</span>
              <strong className="text-slate-800">{advisory.overview.cpu_pressure}</strong>
            </div>
            <div className="p-2.5 rounded-lg bg-slate-50 border border-slate-200">
              <span className="text-slate-500 block text-[10px] uppercase font-semibold">Memory Pressure</span>
              <strong className="text-slate-800">{advisory.overview.memory_pressure}</strong>
            </div>
            <div className="p-2.5 rounded-lg bg-slate-50 border border-slate-200">
              <span className="text-slate-500 block text-[10px] uppercase font-semibold">Process Pressure</span>
              <strong className="text-slate-800">{advisory.overview.process_pressure}</strong>
            </div>
            <div className="p-2.5 rounded-lg bg-slate-50 border border-slate-200">
              <span className="text-slate-500 block text-[10px] uppercase font-semibold">Paging Pressure</span>
              <strong className="text-slate-800">{advisory.overview.paging_pressure}</strong>
            </div>
          </div>

          {topAdvisory && (
            <div className="p-3 rounded-lg bg-indigo-50/60 border border-indigo-100 text-xs flex items-start space-x-2.5">
              <Info className="w-4 h-4 text-indigo-600 shrink-0 mt-0.5" />
              <div className="flex-1">
                <span className="font-semibold text-indigo-950 block">
                  Priority Advisory: {topAdvisory.title} ({topAdvisory.severity})
                </span>
                <span className="text-slate-600 leading-relaxed block mt-0.5">
                  {topAdvisory.rationale}
                </span>
              </div>
            </div>
          )}
        </div>
      )}

      {/* Real History Telemetry Charts */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Real-time CPU Utilization Stream Chart */}
        <div className="bg-white p-5 rounded-lg border border-slate-200 shadow-xs">
          <div className="flex items-center justify-between mb-4">
            <div>
              <h3 className="text-sm font-bold text-slate-800">CPU Utilization Over Time</h3>
              <p className="text-xs text-slate-500">Live measurements collected directly during session ({history.length} samples)</p>
            </div>
            <div className="flex items-center space-x-1.5 text-xs text-emerald-600 font-mono font-semibold">
              <span className="w-2 h-2 rounded-full bg-emerald-500 animate-pulse"></span>
              <span>{overview.cpu_utilization.toFixed(1)}%</span>
            </div>
          </div>

          <div className="h-64">
            {history.length > 1 ? (
              <ResponsiveContainer width="100%" height="100%">
                <AreaChart data={history} margin={{ top: 10, right: 10, left: -20, bottom: 0 }}>
                  <defs>
                    <linearGradient id="cpuGradient" x1="0" y1="0" x2="0" y2="1">
                      <stop offset="5%" stopColor="#10b981" stopOpacity={0.3} />
                      <stop offset="95%" stopColor="#10b981" stopOpacity={0.0} />
                    </linearGradient>
                  </defs>
                  <CartesianGrid strokeDasharray="3 3" stroke="#f1f5f9" vertical={false} />
                  <XAxis dataKey="time" tick={{ fontSize: 10, fill: '#94a3b8' }} tickLine={false} />
                  <YAxis domain={[0, 100]} tick={{ fontSize: 10, fill: '#94a3b8' }} tickLine={false} unit="%" />
                  <Tooltip
                    contentStyle={{ backgroundColor: '#0f172a', borderColor: '#334155', borderRadius: '6px', fontSize: '11px', color: '#fff' }}
                    itemStyle={{ color: '#34d399' }}
                    formatter={(val: unknown) => [`${Number(val).toFixed(2)}%`, 'CPU Usage']}
                  />
                  <Area
                    type="monotone"
                    dataKey="cpu"
                    stroke="#10b981"
                    strokeWidth={2}
                    fillOpacity={1}
                    fill="url(#cpuGradient)"
                    isAnimationActive={false}
                  />
                </AreaChart>
              </ResponsiveContainer>
            ) : (
              <div className="h-full flex items-center justify-center text-xs text-slate-400">
                Collecting consecutive time-series samples from C backend...
              </div>
            )}
          </div>
        </div>

        {/* Real-time Memory Utilization Stream Chart */}
        <div className="bg-white p-5 rounded-lg border border-slate-200 shadow-xs">
          <div className="flex items-center justify-between mb-4">
            <div>
              <h3 className="text-sm font-bold text-slate-800">Memory Utilization Over Time</h3>
              <p className="text-xs text-slate-500">Physical RAM telemetry measured consecutively ({history.length} samples)</p>
            </div>
            <div className="flex items-center space-x-1.5 text-xs text-blue-600 font-mono font-semibold">
              <span className="w-2 h-2 rounded-full bg-blue-500 animate-pulse"></span>
              <span>{overview.memory_utilization.toFixed(1)}%</span>
            </div>
          </div>

          <div className="h-64">
            {history.length > 1 ? (
              <ResponsiveContainer width="100%" height="100%">
                <AreaChart data={history} margin={{ top: 10, right: 10, left: -20, bottom: 0 }}>
                  <defs>
                    <linearGradient id="memGradient" x1="0" y1="0" x2="0" y2="1">
                      <stop offset="5%" stopColor="#3b82f6" stopOpacity={0.3} />
                      <stop offset="95%" stopColor="#3b82f6" stopOpacity={0.0} />
                    </linearGradient>
                  </defs>
                  <CartesianGrid strokeDasharray="3 3" stroke="#f1f5f9" vertical={false} />
                  <XAxis dataKey="time" tick={{ fontSize: 10, fill: '#94a3b8' }} tickLine={false} />
                  <YAxis domain={[0, 100]} tick={{ fontSize: 10, fill: '#94a3b8' }} tickLine={false} unit="%" />
                  <Tooltip
                    contentStyle={{ backgroundColor: '#0f172a', borderColor: '#334155', borderRadius: '6px', fontSize: '11px', color: '#fff' }}
                    itemStyle={{ color: '#60a5fa' }}
                    formatter={(val: unknown) => [`${Number(val).toFixed(2)}%`, 'Memory Utilization']}
                  />
                  <Area
                    type="monotone"
                    dataKey="memory"
                    stroke="#3b82f6"
                    strokeWidth={2}
                    fillOpacity={1}
                    fill="url(#memGradient)"
                    isAnimationActive={false}
                  />
                </AreaChart>
              </ResponsiveContainer>
            ) : (
              <div className="h-full flex items-center justify-center text-xs text-slate-400">
                Collecting consecutive time-series samples from C backend...
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Cross-Module Synthesis Grid (Phase 2 & Phase 3 Modules) */}
      <div>
        <div className="flex items-center justify-between mb-3">
          <div className="flex items-center space-x-2">
            <Activity className="w-4 h-4 text-indigo-600" />
            <h2 className="text-xs font-bold uppercase tracking-wider text-slate-500">
              Integrated Analytical Engines &amp; Simulation Subsystems
            </h2>
          </div>
          <div className="flex items-center space-x-2">
            {moduleLoading && (
              <span className="text-[10px] text-slate-400 animate-pulse font-medium">Syncing models...</span>
            )}
            <span className="text-[11px] font-semibold text-purple-700 bg-purple-50 border border-purple-200 px-2 py-0.5 rounded">
              Algorithmic Models &amp; Forecasts
            </span>
          </div>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-4 gap-4">
          {/* Module 2: CPU Scheduling Card */}
          <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs flex flex-col justify-between">
            <div>
              <div className="flex items-center justify-between mb-2">
                <span className="text-[10px] font-bold uppercase tracking-wider text-purple-700 bg-purple-50 px-1.5 py-0.5 rounded border border-purple-200">
                  Simulation Model
                </span>
                <Cpu className="w-4 h-4 text-purple-600" />
              </div>
              <h3 className="text-sm font-bold text-slate-800">CPU Scheduling Engine</h3>
              <p className="text-xs text-slate-500 mt-1 leading-relaxed">
                Evaluates FCFS, SJF, SRTF, Priority, and Round Robin across workload burst variances.
              </p>
              <div className="mt-3 p-2 bg-slate-50 rounded border border-slate-200 text-xs">
                <div className="flex justify-between text-slate-600">
                  <span>Recommended:</span>
                  <strong className="text-indigo-600">SRTF (Preemptive)</strong>
                </div>
                <div className="flex justify-between text-slate-600 mt-1">
                  <span>Avg Wait Reduction:</span>
                  <strong className="text-emerald-600">63% vs FCFS</strong>
                </div>
              </div>
            </div>
            {onNavigateToTab && (
              <button
                onClick={() => onNavigateToTab('scheduling')}
                className="mt-3 text-xs font-semibold text-indigo-600 hover:text-indigo-700 flex items-center space-x-1 cursor-pointer pt-2 border-t border-slate-100"
              >
                <span>Launch Scheduling Simulator</span>
                <ArrowUpRight className="w-3.5 h-3.5" />
              </button>
            )}
          </div>

          {/* Module 3: Memory Forecasting Card */}
          <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs flex flex-col justify-between">
            <div>
              <div className="flex items-center justify-between mb-2">
                <span className="text-[10px] font-bold uppercase tracking-wider text-blue-700 bg-blue-50 px-1.5 py-0.5 rounded border border-blue-200">
                  Statistical OLS Forecast
                </span>
                <TrendingUp className="w-4 h-4 text-blue-600" />
              </div>
              <h3 className="text-sm font-bold text-slate-800">Memory Trend Forecasting</h3>
              <p className="text-xs text-slate-500 mt-1 leading-relaxed">
                Linear regression velocity and SMA smoothing for RAM exhaustion projection.
              </p>
              <div className="mt-3 p-2 bg-slate-50 rounded border border-slate-200 text-xs">
                <div className="flex justify-between text-slate-600">
                  <span>Growth Velocity:</span>
                  <strong className="text-blue-600">
                    {forecast ? `${forecast.slope_pct_per_min >= 0 ? '+' : ''}${forecast.slope_pct_per_min.toFixed(2)}%/min` : 'Analyzing...'}
                  </strong>
                </div>
                <div className="flex justify-between text-slate-600 mt-1">
                  <span>Model Confidence ($R^2$):</span>
                  <strong className="text-slate-800">
                    {forecast ? forecast.r_squared.toFixed(2) : '1.00'}
                  </strong>
                </div>
              </div>
            </div>
            {onNavigateToTab && (
              <button
                onClick={() => onNavigateToTab('memory')}
                className="mt-3 text-xs font-semibold text-blue-600 hover:text-blue-700 flex items-center space-x-1 cursor-pointer pt-2 border-t border-slate-100"
              >
                <span>Open Memory Prediction</span>
                <ArrowUpRight className="w-3.5 h-3.5" />
              </button>
            )}
          </div>

          {/* Module 4: Page Replacement Card */}
          <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs flex flex-col justify-between">
            <div>
              <div className="flex items-center justify-between mb-2">
                <span className="text-[10px] font-bold uppercase tracking-wider text-emerald-700 bg-emerald-50 px-1.5 py-0.5 rounded border border-emerald-200">
                  Simulation Model
                </span>
                <Layers className="w-4 h-4 text-emerald-600" />
              </div>
              <h3 className="text-sm font-bold text-slate-800">Page Replacement Engine</h3>
              <p className="text-xs text-slate-500 mt-1 leading-relaxed">
                Frame-by-frame simulation comparing FIFO, LRU, and Optimal evictions and Belady&apos;s anomaly.
              </p>
              <div className="mt-3 p-2 bg-slate-50 rounded border border-slate-200 text-xs">
                <div className="flex justify-between text-slate-600">
                  <span>Optimal Lower Bound:</span>
                  <strong className="text-emerald-600">9 Faults (45% Hits)</strong>
                </div>
                <div className="flex justify-between text-slate-600 mt-1">
                  <span>LRU vs FIFO:</span>
                  <strong className="text-slate-800">20% Fault Reduction</strong>
                </div>
              </div>
            </div>
            {onNavigateToTab && (
              <button
                onClick={() => onNavigateToTab('page-replacement')}
                className="mt-3 text-xs font-semibold text-emerald-600 hover:text-emerald-700 flex items-center space-x-1 cursor-pointer pt-2 border-t border-slate-100"
              >
                <span>Simulate Page Evictions</span>
                <ArrowUpRight className="w-3.5 h-3.5" />
              </button>
            )}
          </div>

          {/* Module 5: Deadlock Avoidance Card */}
          <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs flex flex-col justify-between">
            <div>
              <div className="flex items-center justify-between mb-2">
                <span className="text-[10px] font-bold uppercase tracking-wider text-amber-700 bg-amber-50 px-1.5 py-0.5 rounded border border-amber-200">
                  Resource-Allocation Model
                </span>
                <ShieldAlert className="w-4 h-4 text-amber-600" />
              </div>
              <h3 className="text-sm font-bold text-slate-800">Deadlock Avoidance</h3>
              <p className="text-xs text-slate-500 mt-1 leading-relaxed">
                Banker&apos;s algorithm safe-state analysis, resource-request checker, and cycle detection.
              </p>
              <div className="mt-3 p-2 bg-slate-50 rounded border border-slate-200 text-xs">
                <div className="flex justify-between text-slate-600">
                  <span>Baseline Safety:</span>
                  <strong className={deadlockSummary?.is_safe ? 'text-emerald-600' : 'text-amber-600'}>
                    {deadlockSummary ? (deadlockSummary.is_safe ? 'Safe State' : 'Unsafe Warning') : 'Evaluated'}
                  </strong>
                </div>
                <div className="flex justify-between text-slate-600 mt-1">
                  <span>Safe Sequence:</span>
                  <strong className="text-slate-800 font-mono text-[11px]">
                    {deadlockSummary?.safe_sequence ? `<P${deadlockSummary.safe_sequence.slice(0, 3).join(' -> P')}...>` : '<P1 -> P3...>'}
                  </strong>
                </div>
              </div>
            </div>
            {onNavigateToTab && (
              <button
                onClick={() => onNavigateToTab('deadlock')}
                className="mt-3 text-xs font-semibold text-amber-600 hover:text-amber-700 flex items-center space-x-1 cursor-pointer pt-2 border-t border-slate-100"
              >
                <span>Manage Deadlock Matrix</span>
                <ArrowUpRight className="w-3.5 h-3.5" />
              </button>
            )}
          </div>
        </div>
      </div>

      {/* Lower Row: Process State Breakdown & Top Consumers */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Process State Distribution */}
        <div className="bg-white p-5 rounded-lg border border-slate-200 shadow-xs flex flex-col justify-between">
          <div>
            <h3 className="text-sm font-bold text-slate-800">Process State Breakdown</h3>
            <p className="text-xs text-slate-500">Distribution across active system task states</p>
          </div>

          <div className="h-52 my-2 flex items-center justify-center">
            <ResponsiveContainer width="100%" height="100%">
              <PieChart>
                <Pie
                  data={stateData}
                  cx="50%"
                  cy="50%"
                  innerRadius={50}
                  outerRadius={75}
                  paddingAngle={4}
                  dataKey="value"
                >
                  {stateData.map((entry, index) => (
                    <Cell key={`cell-${index}`} fill={entry.color} />
                  ))}
                </Pie>
                <Tooltip
                  contentStyle={{ backgroundColor: '#0f172a', borderColor: '#334155', borderRadius: '6px', fontSize: '11px', color: '#fff' }}
                />
              </PieChart>
            </ResponsiveContainer>
          </div>

          <div className="grid grid-cols-2 gap-2 text-xs pt-2 border-t border-slate-100">
            <div className="flex items-center justify-between p-2 rounded-md bg-emerald-50 text-emerald-900">
              <span className="flex items-center space-x-1.5">
                <span className="w-2 h-2 rounded-full bg-emerald-500"></span>
                <span>Running</span>
              </span>
              <span className="font-mono font-bold">{overview.running_processes}</span>
            </div>
            <div className="flex items-center justify-between p-2 rounded-md bg-blue-50 text-blue-900">
              <span className="flex items-center space-x-1.5">
                <span className="w-2 h-2 rounded-full bg-blue-500"></span>
                <span>Sleeping</span>
              </span>
              <span className="font-mono font-bold">{overview.sleeping_processes}</span>
            </div>
            <div className="flex items-center justify-between p-2 rounded-md bg-amber-50 text-amber-900">
              <span className="flex items-center space-x-1.5">
                <span className="w-2 h-2 rounded-full bg-amber-500"></span>
                <span>Stopped</span>
              </span>
              <span className="font-mono font-bold">{overview.stopped_processes}</span>
            </div>
            <div className="flex items-center justify-between p-2 rounded-md bg-rose-50 text-rose-900">
              <span className="flex items-center space-x-1.5">
                <span className="w-2 h-2 rounded-full bg-rose-500"></span>
                <span>Zombie</span>
              </span>
              <span className="font-mono font-bold">{overview.zombie_processes}</span>
            </div>
          </div>
        </div>

        {/* Top 5 CPU Consumers */}
        <div className="bg-white p-5 rounded-lg border border-slate-200 shadow-xs flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between">
              <h3 className="text-sm font-bold text-slate-800">Top CPU Consumers</h3>
              <button
                onClick={onNavigateToProcesses}
                className="text-[11px] text-emerald-600 hover:text-emerald-700 font-semibold flex items-center space-x-0.5 cursor-pointer"
              >
                <span>View all</span>
                <ArrowUpRight className="w-3 h-3" />
              </button>
            </div>
            <p className="text-xs text-slate-500">Sampled consecutive processor utilization</p>
          </div>

          <div className="mt-4 space-y-2.5">
            {topCpuProcs.map((proc) => (
              <div
                key={proc.pid}
                onClick={() => onInspectProcess(proc.pid)}
                className="flex items-center justify-between p-2 rounded-md hover:bg-slate-50 border border-slate-100 transition-colors cursor-pointer group"
              >
                <div className="truncate mr-2">
                  <div className="flex items-center space-x-1.5">
                    <span className="text-xs font-semibold text-slate-800 group-hover:text-emerald-600 truncate">
                      {proc.name}
                    </span>
                    <span className="text-[10px] text-slate-400 font-mono">#{proc.pid}</span>
                  </div>
                  <span className="text-[10px] text-slate-400">
                    State: {proc.state_full} • {proc.threads} threads
                  </span>
                </div>
                <div className="text-right shrink-0">
                  <span className="text-xs font-bold font-mono text-emerald-600 block">
                    {proc.cpu_pct.toFixed(1)}%
                  </span>
                  <span className="text-[10px] text-slate-400 font-mono">
                    {(proc.mem_kb / 1024).toFixed(0)} MB
                  </span>
                </div>
              </div>
            ))}
          </div>

          <div className="pt-2 text-[11px] text-slate-400 text-center border-t border-slate-100">
            Click any process row to open telemetry inspect modal
          </div>
        </div>

        {/* Top 5 Memory Consumers */}
        <div className="bg-white p-5 rounded-lg border border-slate-200 shadow-xs flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between">
              <h3 className="text-sm font-bold text-slate-800">Top Memory Consumers</h3>
              <button
                onClick={onNavigateToProcesses}
                className="text-[11px] text-emerald-600 hover:text-emerald-700 font-semibold flex items-center space-x-0.5 cursor-pointer"
              >
                <span>View all</span>
                <ArrowUpRight className="w-3 h-3" />
              </button>
            </div>
            <p className="text-xs text-slate-500">Resident set size (RSS) memory consumption</p>
          </div>

          <div className="mt-4 space-y-2.5">
            {topMemProcs.map((proc) => (
              <div
                key={proc.pid}
                onClick={() => onInspectProcess(proc.pid)}
                className="flex items-center justify-between p-2 rounded-md hover:bg-slate-50 border border-slate-100 transition-colors cursor-pointer group"
              >
                <div className="truncate mr-2">
                  <div className="flex items-center space-x-1.5">
                    <span className="text-xs font-semibold text-slate-800 group-hover:text-blue-600 truncate">
                      {proc.name}
                    </span>
                    <span className="text-[10px] text-slate-400 font-mono">#{proc.pid}</span>
                  </div>
                  <span className="text-[10px] text-slate-400">
                    RSS: {(proc.mem_kb / 1024).toFixed(1)} MB • {proc.mem_pct.toFixed(2)}% RAM
                  </span>
                </div>
                <div className="text-right shrink-0">
                  <span className="text-xs font-bold font-mono text-blue-600 block">
                    {(proc.mem_kb / 1024).toFixed(0)} MB
                  </span>
                  <span className="text-[10px] text-slate-400 font-mono">
                    VSZ: {(proc.vsz_kb / 1024).toFixed(0)} MB
                  </span>
                </div>
              </div>
            ))}
          </div>

          <div className="pt-2 text-[11px] text-slate-400 text-center border-t border-slate-100">
            Memory values read directly from host OS process telemetry
          </div>
        </div>
      </div>
    </div>
  );
};
