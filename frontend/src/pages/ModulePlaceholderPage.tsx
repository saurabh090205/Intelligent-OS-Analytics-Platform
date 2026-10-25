import React from 'react';
import {
  Cpu,
  Database,
  ShieldAlert,
  Layers,
  FileText,
  Sliders,
  Calendar,
  Code2,
  Lock,
  Sparkles
} from 'lucide-react';
import type { NavItemKey } from '../components/Sidebar';

interface ModulePlaceholderPageProps {
  moduleKey: NavItemKey;
}

interface ModuleSpec {
  title: string;
  phase: string;
  icon: React.ComponentType<{ className?: string }>;
  description: string;
  theory: string[];
  algorithms: string[];
  plannedEndpoints: string[];
  previewCards: { title: string; desc: string; inputs: string[] }[];
}

const MODULE_SPECS: Partial<Record<NavItemKey, ModuleSpec>> = {
  scheduling: {
    title: 'Module 2: CPU Scheduling Recommendation',
    phase: 'Phase 2',
    icon: Cpu,
    description: 'A comprehensive C simulation and recommendation engine for single-processor scheduling workloads with Gantt chart generation.',
    theory: [
      'Evaluation of turnaround time (TAT = Completion - Arrival) and waiting time (WT = TAT - Burst).',
      'Comparative workload simulation across non-preemptive and preemptive algorithms.',
      'Explainable recommendation engine selecting optimal algorithm based on workload characteristics.'
    ],
    algorithms: [
      'First Come First Serve (FCFS)',
      'Shortest Job First (SJF, non-preemptive)',
      'Shortest Remaining Time First (SRTF, preemptive)',
      'Priority Scheduling (Preemptive & Non-preemptive)',
      'Round Robin (RR, with configurable time quantum)'
    ],
    plannedEndpoints: [
      'POST /api/scheduling/simulate - Run batch simulation on workload',
      'GET /api/scheduling/algorithms - List supported scheduling policies'
    ],
    previewCards: [
      {
        title: 'Workload Input Matrix',
        desc: 'Interactive process arrival, burst time, and priority configuration.',
        inputs: ['Process ID (P1, P2...)', 'Arrival Time (ms)', 'CPU Burst Time (ms)', 'Priority Level', 'Time Quantum']
      },
      {
        title: 'Gantt Chart & Comparative Metrics',
        desc: 'Visual timeline with average waiting time, turnaround time, and throughput graphs.',
        inputs: ['Timeline Visualization', 'Avg Waiting Time Comparison', 'Avg Turnaround Time', 'CPU Utilization %']
      }
    ]
  },
  memory: {
    title: 'Module 3: Memory Usage Prediction',
    phase: 'Phase 2',
    icon: Database,
    description: 'Statistical forecasting engine implemented in C using moving averages and linear regression on collected physical memory telemetry.',
    theory: [
      'Time-series forecasting based on true consecutive /proc/meminfo samples.',
      'Moving average window smoothing to eliminate transient spike artifacts.',
      'Linear trend slope estimation (dy/dt) to predict memory exhaustion time.'
    ],
    algorithms: [
      'Simple Moving Average (SMA, window N=5..30)',
      'Simple Linear Regression (Least Squares Trendline)',
      'Threshold Pressure Warning (> 85% utilization)'
    ],
    plannedEndpoints: [
      'GET /api/memory/forecast?horizon=60 - Retrieve future forecast points',
      'GET /api/memory/trend - Get current memory slope and pressure state'
    ],
    previewCards: [
      {
        title: 'Forecasting Parameters',
        desc: 'Configure statistical window and projection horizon.',
        inputs: ['Sample Window Size (N)', 'Forecast Horizon (Seconds)', 'Smoothing Factor (Alpha)', 'High-Memory Alert Threshold']
      },
      {
        title: 'Predictive Trend Visualization',
        desc: 'Historical actual memory chart layered with linear regression forecast corridor.',
        inputs: ['Actual Telemetry Curve', 'Regression Projection Band', 'Estimated Time to Exhaustion', 'Available RAM Estimate']
      }
    ]
  },
  'page-replacement': {
    title: 'Module 4: Page Replacement Simulation',
    phase: 'Phase 2',
    icon: Layers,
    description: 'Frame-by-frame memory page replacement simulator implemented in C for virtual memory analysis.',
    theory: [
      'Simulation of demand paging with fixed physical frame allocation.',
      'Hit/Fault ratio calculation: Fault Ratio = Faults / Total References.',
      'Belady\'s Anomaly demonstration comparing FIFO vs Optimal.'
    ],
    algorithms: [
      'First-In First-Out (FIFO)',
      'Least Recently Used (LRU, stack/timestamp implementation)',
      'Optimal Page Replacement (Belady\'s MIN / OPT algorithm)'
    ],
    plannedEndpoints: [
      'POST /api/page-replacement/simulate - Run page replacement comparison'
    ],
    previewCards: [
      {
        title: 'Reference String & Frame Buffer',
        desc: 'Specify page reference sequence and physical memory frame capacity.',
        inputs: ['Reference String (e.g. 7, 0, 1, 2, 0, 3, 0, 4)', 'Number of Frames (3 to 8)', 'Algorithm Selection']
      },
      {
        title: 'Frame State Matrix & Fault Tracker',
        desc: 'Step-by-step matrix visualization showing hit/miss events for every memory access.',
        inputs: ['Frame Snapshot Table', 'Page Fault Summary', 'Hit Ratio %', 'Algorithm Comparison Chart']
      }
    ]
  },
  deadlock: {
    title: 'Module 5: Deadlock Prediction & Detection',
    phase: 'Phase 3',
    icon: ShieldAlert,
    description: 'Banker\'s Safety Algorithm and Resource Allocation Graph (RAG) analysis engine written in C.',
    theory: [
      'Safe state validation using Dijkstra\'s Banker\'s Algorithm: Need[i][j] = Max[i][j] - Allocation[i][j].',
      'Cycle detection in wait-for graphs for actual deadlock detection.',
      'Clear separation of safe-state analysis from real-world kernel deadlock prediction.'
    ],
    algorithms: [
      'Banker\'s Safety Algorithm (Safe sequence generator)',
      'Banker\'s Resource Request Algorithm',
      'Resource Allocation Graph Cycle Detection'
    ],
    plannedEndpoints: [
      'POST /api/deadlock/safety-check - Validate system safe state',
      'POST /api/deadlock/request-check - Check if resource grant maintains safety'
    ],
    previewCards: [
      {
        title: 'Resource Allocation Matrix',
        desc: 'Configure process count, resource vector, allocation, and maximum demand matrices.',
        inputs: ['Available Resource Vector [R1, R2...]', 'Allocation Matrix', 'Maximum Demand Matrix', 'Need Matrix (Computed)']
      },
      {
        title: 'Safety Analysis & Safe Sequence',
        desc: 'Step-by-step safety algorithm execution and safe execution order visualization.',
        inputs: ['Safe Sequence (e.g. P1 -> P3 -> P0)', 'System State Status (Safe / Unsafe)', 'Resource Request Validator']
      }
    ]
  },
  logs: {
    title: 'Module 6: System Log Analysis',
    phase: 'Phase 3',
    icon: FileText,
    description: 'C-based log parser for user-accessible operating system log files with timestamp and severity extraction.',
    theory: [
      'Single-pass line streaming parser avoiding memory bloat on large log files.',
      'Severity token extraction (INFO, WARN, ERROR, CRIT, FATAL).',
      'Error burst spike detection using sliding count windows.'
    ],
    algorithms: [
      'Log Tokenizer & Regex-free fast parser',
      'Severity frequency counter',
      'Duplicate message deduplication & recurring error tracker',
      'Configurable error burst detector'
    ],
    plannedEndpoints: [
      'GET /api/logs/summary - Severity counts and error rate',
      'GET /api/logs/entries?severity=ERROR&search=... - Filtered entries'
    ],
    previewCards: [
      {
        title: 'Log File Selector & Filters',
        desc: 'Select readable log file path without elevated root privileges.',
        inputs: ['Configured Log Path', 'Severity Level Multi-select', 'Keyword Search Filter', 'Time Range Filter']
      },
      {
        title: 'Severity Breakdown & Recurring Spikes',
        desc: 'Real-time severity counts, timeline graph of error rates, and highlighted error lines.',
        inputs: ['Severity Chart (INFO, WARN, ERR)', 'Error Burst Alerts', 'Top Recurring Error Messages', 'Raw Log Viewer']
      }
    ]
  },
  'resource-allocation': {
    title: 'Module 7: AI-Based Resource Allocation',
    phase: 'Phase 3',
    icon: Sliders,
    description: 'Transparent, explainable recommendation engine implementing heuristic and statistical optimization models.',
    theory: [
      'Heuristic evaluation of CPU, memory, and task contention.',
      'Advisory recommendations based on empirical operating system tuning rules.',
      'Strict safety boundary: strictly advisory simulations; never performs destructive kills or kernel mutations.'
    ],
    algorithms: [
      'Multi-variable Resource Contention Evaluator',
      'Dynamic CPU Scheduler Advisory Rule-Base',
      'Memory Pressure Mitigation Heuristics',
      'Before-and-after Workload Simulation Model'
    ],
    plannedEndpoints: [
      'GET /api/recommendations/current - Active system recommendations',
      'POST /api/recommendations/simulate - Simulate resource rebalancing'
    ],
    previewCards: [
      {
        title: 'Active Resource Advisory Cards',
        desc: 'Contextual, explainable recommendations backed by real measurements.',
        inputs: ['CPU Contention Alert', 'Memory Pressure Mitigation', 'Scheduler Policy Advisory', 'Unnecessary Task Advisory']
      },
      {
        title: 'Simulated Reallocation Scenario',
        desc: 'Safe sandboxed simulation comparing before vs after resource distribution.',
        inputs: ['Current Allocation State', 'Proposed Allocation Strategy', 'Predicted Latency Gain', 'Risk Assessment Level']
      }
    ]
  }
};

export const ModulePlaceholderPage: React.FC<ModulePlaceholderPageProps> = ({ moduleKey }) => {
  const spec = MODULE_SPECS[moduleKey];

  if (!spec) {
    return (
      <div className="p-8 bg-white rounded-lg border border-slate-200">
        <h3 className="text-base font-bold text-slate-800">Module Upcoming</h3>
        <p className="text-xs text-slate-500 mt-1">This module is planned for subsequent phases.</p>
      </div>
    );
  }

  const Icon = spec.icon;

  return (
    <div className="space-y-6">
      {/* Module Overview Banner */}
      <div className="bg-white p-6 rounded-lg border border-slate-200 shadow-xs">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 pb-4 border-b border-slate-100">
          <div className="flex items-center space-x-3.5">
            <div className="p-3 rounded-lg bg-slate-100 text-slate-700">
              <Icon className="w-6 h-6" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h3 className="text-lg font-bold text-slate-900">{spec.title}</h3>
                <span className="px-2 py-0.5 rounded text-[10px] font-mono font-semibold bg-amber-100 text-amber-800 border border-amber-300">
                  {spec.phase} Upcoming
                </span>
              </div>
              <p className="text-xs text-slate-500 mt-0.5">{spec.description}</p>
            </div>
          </div>

          <div className="flex items-center space-x-2 text-xs text-slate-500 bg-slate-50 px-3 py-2 rounded-md border border-slate-200 shrink-0">
            <Calendar className="w-4 h-4 text-slate-400" />
            <span>Development Status: <strong>Scheduled for {spec.phase}</strong></span>
          </div>
        </div>

        {/* OS Theory & Algorithms Grid */}
        <div className="grid grid-cols-1 md:grid-cols-2 gap-6 mt-6">
          <div>
            <h4 className="text-xs font-bold text-slate-700 uppercase tracking-wider mb-2.5 flex items-center space-x-1.5">
              <Code2 className="w-3.5 h-3.5 text-emerald-600" />
              <span>Core C Algorithms</span>
            </h4>
            <ul className="space-y-1.5 text-xs text-slate-600">
              {spec.algorithms.map((algo, i) => (
                <li key={i} className="flex items-start space-x-2">
                  <span className="w-1.5 h-1.5 rounded-full bg-emerald-500 mt-1.5 shrink-0" />
                  <span>{algo}</span>
                </li>
              ))}
            </ul>
          </div>

          <div>
            <h4 className="text-xs font-bold text-slate-700 uppercase tracking-wider mb-2.5 flex items-center space-x-1.5">
              <Sparkles className="w-3.5 h-3.5 text-blue-600" />
              <span>Operating System Theory & Principles</span>
            </h4>
            <ul className="space-y-1.5 text-xs text-slate-600">
              {spec.theory.map((item, i) => (
                <li key={i} className="flex items-start space-x-2">
                  <span className="w-1.5 h-1.5 rounded-full bg-blue-500 mt-1.5 shrink-0" />
                  <span>{item}</span>
                </li>
              ))}
            </ul>
          </div>
        </div>

        {/* Planned REST API Endpoints */}
        <div className="mt-6 pt-4 border-t border-slate-100">
          <h4 className="text-xs font-bold text-slate-700 uppercase tracking-wider mb-2">
            Planned C REST Endpoints
          </h4>
          <div className="flex flex-wrap gap-2">
            {spec.plannedEndpoints.map((ep, i) => (
              <span key={i} className="px-2.5 py-1 rounded bg-slate-100 text-slate-700 font-mono text-[11px] border border-slate-200">
                {ep}
              </span>
            ))}
          </div>
        </div>
      </div>

      {/* Planned Controls Wireframe Preview */}
      <div>
        <div className="flex items-center justify-between mb-3">
          <h4 className="text-sm font-bold text-slate-800">Planned Interface Wireframe Preview</h4>
          <span className="text-[11px] text-slate-400 flex items-center space-x-1">
            <Lock className="w-3 h-3" />
            <span>Controls disabled pending {spec.phase} implementation</span>
          </span>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          {spec.previewCards.map((card, i) => (
            <div key={i} className="p-5 bg-white rounded-lg border border-dashed border-slate-300 shadow-2xs relative overflow-hidden">
              <div className="flex items-center justify-between mb-2">
                <h5 className="text-xs font-bold text-slate-800">{card.title}</h5>
                <span className="text-[10px] px-2 py-0.5 rounded bg-slate-100 text-slate-500 font-mono">
                  Disabled
                </span>
              </div>
              <p className="text-xs text-slate-500 mb-4">{card.desc}</p>

              <div className="space-y-2 opacity-60 pointer-events-none">
                {card.inputs.map((inp, idx) => (
                  <div key={idx} className="flex items-center justify-between p-2 bg-slate-50 border border-slate-200 rounded text-xs text-slate-600 font-mono">
                    <span>{inp}</span>
                    <span className="text-[10px] text-slate-400">Waiting for {spec.phase}...</span>
                  </div>
                ))}
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};
