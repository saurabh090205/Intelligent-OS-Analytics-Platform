import React, { useState, useEffect, useCallback } from 'react';
import {
  Cpu,
  Play,
  RotateCcw,
  Plus,
  Trash2,
  Sparkles,
  BarChart3,
  Clock,
  CheckCircle2,
  AlertTriangle,
  Info
} from 'lucide-react';
import { api } from '../services/api';
import type {
  SchedProcessInput,
  SchedSimulationResponse,
  SchedResult,
  SchedExample
} from '../types/system';

const DEFAULT_PROCESSES: SchedProcessInput[] = [
  { pid: 1, name: 'P1', arrival_time: 0, burst_time: 8, priority: 3 },
  { pid: 2, name: 'P2', arrival_time: 1, burst_time: 4, priority: 1 },
  { pid: 3, name: 'P3', arrival_time: 2, burst_time: 9, priority: 4 },
  { pid: 4, name: 'P4', arrival_time: 3, burst_time: 5, priority: 2 },
];

const PROCESS_COLORS = [
  'bg-emerald-500 text-white',
  'bg-blue-500 text-white',
  'bg-purple-500 text-white',
  'bg-amber-500 text-white',
  'bg-rose-500 text-white',
  'bg-indigo-500 text-white',
  'bg-cyan-500 text-white',
  'bg-orange-500 text-white',
];

export const CpuSchedulingPage: React.FC = () => {
  const [processes, setProcesses] = useState<SchedProcessInput[]>(DEFAULT_PROCESSES);
  const [quantum, setQuantum] = useState<number>(3);
  const [examples, setExamples] = useState<SchedExample[]>([]);
  const [simulation, setSimulation] = useState<SchedSimulationResponse | null>(null);
  const [activeAlgoKey, setActiveAlgoKey] = useState<keyof SchedSimulationResponse['algorithms']>('srtf');
  const [isLoading, setIsLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);

  // Load backend examples on mount
  useEffect(() => {
    api.getSchedulingExamples()
      .then((res) => {
        if (res.examples && res.examples.length > 0) {
          setExamples(res.examples);
        }
      })
      .catch(() => {
        // Fallback silently if offline
      });
  }, []);

  const runSimulation = useCallback(async (currentProcs: SchedProcessInput[], q: number) => {
    setIsLoading(true);
    setError(null);
    try {
      const validProcs = currentProcs.filter((p) => p.burst_time > 0);
      if (validProcs.length === 0) {
        setError('At least one process with burst time > 0 is required.');
        setIsLoading(false);
        return;
      }

      const res = await api.simulateScheduling({
        processes: validProcs,
        quantum: q,
      });

      setSimulation(res);
      // Default selected tab to recommended algorithm
      if (res.recommended_algo_id === 0) setActiveAlgoKey('fcfs');
      else if (res.recommended_algo_id === 1) setActiveAlgoKey('sjf');
      else if (res.recommended_algo_id === 2) setActiveAlgoKey('srtf');
      else if (res.recommended_algo_id === 3) setActiveAlgoKey('priority');
      else if (res.recommended_algo_id === 4) setActiveAlgoKey('round_robin');
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Simulation failed');
    } finally {
      setIsLoading(false);
    }
  }, []);

  // Initial simulation run
  useEffect(() => {
    runSimulation(processes, quantum);
  }, [runSimulation]);

  const handleAddProcess = () => {
    const nextPid = processes.length > 0 ? Math.max(...processes.map((p) => p.pid)) + 1 : 1;
    setProcesses([
      ...processes,
      {
        pid: nextPid,
        name: `P${nextPid}`,
        arrival_time: processes.length > 0 ? processes[processes.length - 1].arrival_time + 1 : 0,
        burst_time: 4,
        priority: 2,
      },
    ]);
  };

  const handleRemoveProcess = (pid: number) => {
    if (processes.length <= 1) {
      setError('At least one process must remain.');
      return;
    }
    const filtered = processes.filter((p) => p.pid !== pid);
    setProcesses(filtered);
    runSimulation(filtered, quantum);
  };

  const handleUpdateProcess = (pid: number, field: keyof SchedProcessInput, value: string | number) => {
    const updated = processes.map((p) => {
      if (p.pid === pid) {
        return { ...p, [field]: value };
      }
      return p;
    });
    setProcesses(updated);
  };

  const handleLoadExample = (ex: SchedExample) => {
    if (ex.processes && ex.processes.length > 0) {
      setProcesses(ex.processes);
      setQuantum(ex.quantum || 2);
      runSimulation(ex.processes, ex.quantum || 2);
    }
  };

  const currentResult: SchedResult | undefined = simulation?.algorithms[activeAlgoKey];

  return (
    <div className="space-y-6">
      {/* Header Banner */}
      <div className="bg-white rounded-xl border border-slate-200 p-6 shadow-2xs">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div className="flex items-center space-x-3">
            <div className="w-10 h-10 rounded-lg bg-emerald-500/10 border border-emerald-500/20 flex items-center justify-center text-emerald-600">
              <Cpu className="w-6 h-6" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h1 className="text-xl font-bold text-slate-900 tracking-tight">CPU Scheduling Simulator & Recommendation Engine</h1>
                <span className="px-2 py-0.5 text-xs font-semibold rounded bg-emerald-100 text-emerald-800">
                  C Backend POSIX
                </span>
              </div>
              <p className="text-xs text-slate-500 mt-0.5">
                Simulates FCFS, SJF, SRTF, Priority, and Round Robin in C with explainable algorithmic recommendations.
              </p>
            </div>
          </div>

          {/* Action Buttons */}
          <div className="flex items-center space-x-2">
            <button
              onClick={() => runSimulation(processes, quantum)}
              disabled={isLoading}
              className="px-4 py-2 bg-emerald-600 hover:bg-emerald-700 text-white rounded-lg text-xs font-semibold flex items-center space-x-2 shadow-xs transition-colors cursor-pointer disabled:opacity-50"
            >
              <Play className="w-3.5 h-3.5 fill-current" />
              <span>{isLoading ? 'Simulating...' : 'Run Simulation'}</span>
            </button>
            <button
              onClick={() => {
                setProcesses(DEFAULT_PROCESSES);
                setQuantum(3);
                runSimulation(DEFAULT_PROCESSES, 3);
              }}
              className="px-3 py-2 bg-slate-100 hover:bg-slate-200 text-slate-700 rounded-lg text-xs font-semibold flex items-center space-x-1.5 transition-colors cursor-pointer"
            >
              <RotateCcw className="w-3.5 h-3.5" />
              <span>Reset</span>
            </button>
          </div>
        </div>

        {/* Preset Workload Examples */}
        {examples.length > 0 && (
          <div className="mt-4 pt-4 border-t border-slate-100 flex flex-wrap items-center gap-2">
            <span className="text-xs font-medium text-slate-500 flex items-center gap-1">
              <Sparkles className="w-3.5 h-3.5 text-amber-500" /> Presets:
            </span>
            {examples.map((ex) => (
              <button
                key={ex.id}
                onClick={() => handleLoadExample(ex)}
                title={ex.description}
                className="px-2.5 py-1 text-xs rounded-md border border-slate-200 hover:border-emerald-300 hover:bg-emerald-50/50 text-slate-700 transition-colors cursor-pointer"
              >
                {ex.title}
              </button>
            ))}
          </div>
        )}
      </div>

      {error && (
        <div className="bg-rose-50 border border-rose-200 rounded-lg p-3 text-xs text-rose-700 flex items-center space-x-2">
          <AlertTriangle className="w-4 h-4 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      {/* Recommendation Banner */}
      {simulation && (
        <div className="bg-gradient-to-r from-emerald-900 to-slate-900 rounded-xl p-5 text-white shadow-md border border-emerald-700/40">
          <div className="flex items-start space-x-3">
            <div className="p-2 rounded-lg bg-emerald-500/20 border border-emerald-400/30 text-emerald-300 shrink-0">
              <Sparkles className="w-5 h-5" />
            </div>
            <div className="space-y-1">
              <div className="flex items-center space-x-2">
                <span className="text-[10px] font-bold uppercase tracking-wider text-emerald-400 bg-emerald-950/80 px-2 py-0.5 rounded border border-emerald-500/30">
                  Recommended Algorithm
                </span>
                <span className="text-sm font-bold text-white">
                  {simulation.recommended_algo_name}
                </span>
              </div>
              <p className="text-xs text-slate-200 leading-relaxed pt-1">
                {simulation.recommendation_reason}
              </p>
            </div>
          </div>
        </div>
      )}

      {/* Main Grid: Process Table & Comparison Cards */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Left Column: Process Workload Input Form (1 col) */}
        <div className="bg-white rounded-xl border border-slate-200 p-5 shadow-2xs space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="text-sm font-bold text-slate-900 flex items-center space-x-2">
              <Clock className="w-4 h-4 text-emerald-600" />
              <span>Workload Processes</span>
            </h2>
            <button
              onClick={handleAddProcess}
              className="px-2 py-1 bg-emerald-50 hover:bg-emerald-100 text-emerald-700 border border-emerald-200 rounded text-xs font-medium flex items-center space-x-1 cursor-pointer transition-colors"
            >
              <Plus className="w-3 h-3" />
              <span>Add Process</span>
            </button>
          </div>

          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs">
              <thead className="bg-slate-50 text-slate-500 uppercase text-[10px] font-semibold border-b border-slate-200">
                <tr>
                  <th className="py-2 px-1">Name</th>
                  <th className="py-2 px-1">Arrival</th>
                  <th className="py-2 px-1">Burst</th>
                  <th className="py-2 px-1">Pri</th>
                  <th className="py-2 px-1 text-center">Action</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100">
                {processes.map((p) => (
                  <tr key={p.pid} className="hover:bg-slate-50/80">
                    <td className="py-1.5 px-1 font-semibold text-slate-800">
                      <input
                        type="text"
                        value={p.name}
                        onChange={(e) => handleUpdateProcess(p.pid, 'name', e.target.value)}
                        className="w-12 px-1 py-0.5 border border-slate-200 rounded text-xs focus:ring-1 focus:ring-emerald-500"
                      />
                    </td>
                    <td className="py-1.5 px-1">
                      <input
                        type="number"
                        min="0"
                        value={p.arrival_time}
                        onChange={(e) => handleUpdateProcess(p.pid, 'arrival_time', parseInt(e.target.value) || 0)}
                        className="w-14 px-1 py-0.5 border border-slate-200 rounded text-xs focus:ring-1 focus:ring-emerald-500"
                      />
                    </td>
                    <td className="py-1.5 px-1">
                      <input
                        type="number"
                        min="1"
                        value={p.burst_time}
                        onChange={(e) => handleUpdateProcess(p.pid, 'burst_time', parseInt(e.target.value) || 1)}
                        className="w-14 px-1 py-0.5 border border-slate-200 rounded text-xs focus:ring-1 focus:ring-emerald-500"
                      />
                    </td>
                    <td className="py-1.5 px-1">
                      <input
                        type="number"
                        min="1"
                        max="99"
                        value={p.priority}
                        onChange={(e) => handleUpdateProcess(p.pid, 'priority', parseInt(e.target.value) || 1)}
                        className="w-12 px-1 py-0.5 border border-slate-200 rounded text-xs focus:ring-1 focus:ring-emerald-500"
                      />
                    </td>
                    <td className="py-1.5 px-1 text-center">
                      <button
                        onClick={() => handleRemoveProcess(p.pid)}
                        className="p-1 text-slate-400 hover:text-rose-600 transition-colors cursor-pointer"
                        title="Delete process"
                      >
                        <Trash2 className="w-3.5 h-3.5" />
                      </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          {/* Round Robin Time Quantum setting */}
          <div className="pt-3 border-t border-slate-100 flex items-center justify-between text-xs">
            <span className="font-medium text-slate-700">RR Time Quantum:</span>
            <div className="flex items-center space-x-2">
              <input
                type="number"
                min="1"
                max="20"
                value={quantum}
                onChange={(e) => {
                  const val = Math.max(1, parseInt(e.target.value) || 1);
                  setQuantum(val);
                }}
                className="w-16 px-2 py-1 border border-slate-200 rounded text-xs text-center font-bold text-slate-900"
              />
              <span className="text-slate-400 text-[11px]">ms</span>
            </div>
          </div>
        </div>

        {/* Right Column: Comparative Summary Matrix across all 5 algorithms (2 cols) */}
        <div className="lg:col-span-2 bg-white rounded-xl border border-slate-200 p-5 shadow-2xs space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="text-sm font-bold text-slate-900 flex items-center space-x-2">
              <BarChart3 className="w-4 h-4 text-emerald-600" />
              <span>Algorithm Performance Comparison</span>
            </h2>
            <span className="text-[11px] text-slate-500">Lower Waiting Time is better</span>
          </div>

          {simulation && (
            <div className="overflow-x-auto">
              <table className="w-full text-left text-xs">
                <thead className="bg-slate-50 text-slate-500 uppercase text-[10px] font-semibold border-b border-slate-200">
                  <tr>
                    <th className="py-2.5 px-3">Algorithm</th>
                    <th className="py-2.5 px-2 text-right">Avg WT</th>
                    <th className="py-2.5 px-2 text-right">Avg TAT</th>
                    <th className="py-2.5 px-2 text-right">Avg RT</th>
                    <th className="py-2.5 px-2 text-right">CPU Util</th>
                    <th className="py-2.5 px-3 text-center">Status</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-100">
                  {[
                    { key: 'fcfs' as const, name: 'First Come First Serve (FCFS)', res: simulation.algorithms.fcfs },
                    { key: 'sjf' as const, name: 'Shortest Job First (SJF)', res: simulation.algorithms.sjf },
                    { key: 'srtf' as const, name: 'Shortest Remaining Time First (SRTF)', res: simulation.algorithms.srtf },
                    { key: 'priority' as const, name: 'Priority Scheduling', res: simulation.algorithms.priority },
                    { key: 'round_robin' as const, name: `Round Robin (Q=${quantum})`, res: simulation.algorithms.round_robin },
                  ].map((row) => {
                    const isSelected = activeAlgoKey === row.key;
                    const isRecommended = simulation.recommended_algo_id === row.res.algo_id;

                    return (
                      <tr
                        key={row.key}
                        onClick={() => setActiveAlgoKey(row.key)}
                        className={`cursor-pointer transition-colors ${
                          isSelected ? 'bg-emerald-50/70 font-semibold' : 'hover:bg-slate-50'
                        }`}
                      >
                        <td className="py-2 px-3">
                          <div className="flex items-center space-x-2">
                            <span className={`w-2 h-2 rounded-full ${isSelected ? 'bg-emerald-600' : 'bg-slate-300'}`} />
                            <span className="text-slate-800">{row.name}</span>
                          </div>
                        </td>
                        <td className="py-2 px-2 text-right font-mono font-medium text-slate-900">
                          {row.res.avg_waiting_time.toFixed(2)} ms
                        </td>
                        <td className="py-2 px-2 text-right font-mono text-slate-600">
                          {row.res.avg_turnaround_time.toFixed(2)} ms
                        </td>
                        <td className="py-2 px-2 text-right font-mono text-slate-600">
                          {row.res.avg_response_time.toFixed(2)} ms
                        </td>
                        <td className="py-2 px-2 text-right font-mono text-slate-600">
                          {row.res.cpu_utilization_pct.toFixed(0)}%
                        </td>
                        <td className="py-2 px-3 text-center">
                          {isRecommended ? (
                            <span className="inline-flex items-center px-2 py-0.5 rounded text-[10px] font-bold bg-emerald-100 text-emerald-800">
                              <CheckCircle2 className="w-3 h-3 mr-1" /> Recommended
                            </span>
                          ) : (
                            <span className="text-[10px] text-slate-400">Baseline</span>
                          )}
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>

      {/* Selected Algorithm Results & Gantt Chart */}
      {currentResult && (
        <div className="bg-white rounded-xl border border-slate-200 p-6 shadow-2xs space-y-6">
          {/* Algorithm Tabs */}
          <div className="flex items-center justify-between border-b border-slate-200 pb-3 flex-wrap gap-2">
            <div className="flex space-x-1">
              {[
                { key: 'fcfs' as const, label: 'FCFS' },
                { key: 'sjf' as const, label: 'SJF (Non-Preemptive)' },
                { key: 'srtf' as const, label: 'SRTF (Preemptive)' },
                { key: 'priority' as const, label: 'Priority' },
                { key: 'round_robin' as const, label: `Round Robin (Q=${quantum})` },
              ].map((tab) => (
                <button
                  key={tab.key}
                  onClick={() => setActiveAlgoKey(tab.key)}
                  className={`px-3 py-1.5 text-xs font-semibold rounded-md transition-colors cursor-pointer ${
                    activeAlgoKey === tab.key
                      ? 'bg-slate-900 text-white shadow-xs'
                      : 'text-slate-600 hover:text-slate-900 hover:bg-slate-100'
                  }`}
                >
                  {tab.label}
                </button>
              ))}
            </div>

            <div className="text-xs text-slate-500 flex items-center space-x-1">
              <Info className="w-3.5 h-3.5" />
              <span>Total execution: <strong className="text-slate-800">{currentResult.total_time} ms</strong></span>
            </div>
          </div>

          {/* Interactive Gantt Chart */}
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <h3 className="text-xs font-bold uppercase tracking-wider text-slate-500">
                Timeline Gantt Chart ({currentResult.algo_name})
              </h3>
              <div className="flex items-center space-x-2 text-[11px] text-slate-500">
                <span>Segments: {currentResult.gantt.length}</span>
              </div>
            </div>

            {/* Visual Gantt Bar */}
            <div className="relative border border-slate-200 rounded-lg p-2 bg-slate-50/50">
              <div className="flex h-12 w-full rounded overflow-hidden shadow-inner">
                {currentResult.gantt.map((seg, idx) => {
                  const duration = seg.end_time - seg.start_time;
                  const total = currentResult.total_time || 1;
                  const widthPct = (duration / total) * 100;
                  const colorClass = seg.is_idle
                    ? 'bg-slate-300 text-slate-600'
                    : PROCESS_COLORS[(seg.pid - 1) % PROCESS_COLORS.length];

                  return (
                    <div
                      key={idx}
                      style={{ width: `${widthPct}%` }}
                      className={`h-full flex flex-col items-center justify-center font-bold text-xs border-r border-white/20 transition-transform hover:brightness-110 cursor-pointer ${colorClass}`}
                      title={`${seg.is_idle ? 'IDLE' : seg.name}: ${seg.start_time}ms → ${seg.end_time}ms (${duration}ms)`}
                    >
                      <span className="truncate px-1">{seg.is_idle ? 'IDLE' : seg.name}</span>
                      <span className="text-[10px] font-normal opacity-90">{duration}ms</span>
                    </div>
                  );
                })}
              </div>

              {/* Time axis ticks */}
              <div className="flex justify-between text-[10px] font-mono text-slate-400 mt-2 px-1">
                <span>0 ms</span>
                {currentResult.gantt.map((seg, idx) => (
                  <span key={idx} className="hidden sm:inline-block">
                    {seg.end_time}
                  </span>
                ))}
                <span>{currentResult.total_time} ms</span>
              </div>
            </div>
          </div>

          {/* Process-Level Breakdown Table */}
          <div className="space-y-2">
            <h3 className="text-xs font-bold uppercase tracking-wider text-slate-500">
              Process Scheduling Metrics
            </h3>

            <div className="overflow-x-auto">
              <table className="w-full text-left text-xs border border-slate-200 rounded-lg overflow-hidden">
                <thead className="bg-slate-50 text-slate-600 uppercase text-[10px] font-bold border-b border-slate-200">
                  <tr>
                    <th className="py-2.5 px-3">Process</th>
                    <th className="py-2.5 px-3">Arrival</th>
                    <th className="py-2.5 px-3">Burst</th>
                    <th className="py-2.5 px-3">Priority</th>
                    <th className="py-2.5 px-3">Start Time</th>
                    <th className="py-2.5 px-3">Completion Time</th>
                    <th className="py-2.5 px-3 font-semibold text-emerald-700">Turnaround Time</th>
                    <th className="py-2.5 px-3 font-semibold text-blue-700">Waiting Time</th>
                    <th className="py-2.5 px-3 font-semibold text-purple-700">Response Time</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-100">
                  {currentResult.processes.map((p) => (
                    <tr key={p.pid} className="hover:bg-slate-50">
                      <td className="py-2.5 px-3 font-bold text-slate-900">{p.name}</td>
                      <td className="py-2.5 px-3 font-mono text-slate-600">{p.arrival_time} ms</td>
                      <td className="py-2.5 px-3 font-mono text-slate-600">{p.burst_time} ms</td>
                      <td className="py-2.5 px-3 font-mono text-slate-600">{p.priority}</td>
                      <td className="py-2.5 px-3 font-mono text-slate-600">{p.start_time} ms</td>
                      <td className="py-2.5 px-3 font-mono text-slate-900 font-semibold">{p.completion_time} ms</td>
                      <td className="py-2.5 px-3 font-mono text-emerald-700 font-semibold">{p.turnaround_time} ms</td>
                      <td className="py-2.5 px-3 font-mono text-blue-700 font-semibold">{p.waiting_time} ms</td>
                      <td className="py-2.5 px-3 font-mono text-purple-700 font-semibold">{p.response_time} ms</td>
                    </tr>
                  ))}
                </tbody>
                <tfoot className="bg-slate-50 font-bold text-slate-900 border-t border-slate-200">
                  <tr>
                    <td colSpan={6} className="py-2.5 px-3 text-right uppercase text-[10px] text-slate-500">
                      Workload Average:
                    </td>
                    <td className="py-2.5 px-3 font-mono text-emerald-800">
                      {currentResult.avg_turnaround_time.toFixed(2)} ms
                    </td>
                    <td className="py-2.5 px-3 font-mono text-blue-800">
                      {currentResult.avg_waiting_time.toFixed(2)} ms
                    </td>
                    <td className="py-2.5 px-3 font-mono text-purple-800">
                      {currentResult.avg_response_time.toFixed(2)} ms
                    </td>
                  </tr>
                </tfoot>
              </table>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
