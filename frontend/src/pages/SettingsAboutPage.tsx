import React, { useState } from 'react';
import {
  Server,
  Terminal,
  Code2,
  Play,
  FileCode
} from 'lucide-react';
import type { ApiHealth } from '../types/system';

interface SettingsAboutPageProps {
  health: ApiHealth | null;
}

export const SettingsAboutPage: React.FC<SettingsAboutPageProps> = ({ health }) => {
  const [testEndpoint, setTestEndpoint] = useState<string>('/health');
  const [testLoading, setTestLoading] = useState<boolean>(false);
  const [testResponse, setTestResponse] = useState<string | null>(null);
  const [testLatency, setTestLatency] = useState<number | null>(null);

  const runEndpointTest = async (endpoint: string) => {
    setTestEndpoint(endpoint);
    setTestLoading(true);
    setTestResponse(null);
    setTestLatency(null);

    const start = performance.now();
    try {
      const res = await fetch(`http://127.0.0.1:8080/api${endpoint}`);
      const duration = Math.round(performance.now() - start);
      setTestLatency(duration);

      const json = await res.json();
      setTestResponse(JSON.stringify(json, null, 2));
    } catch (err: unknown) {
      const duration = Math.round(performance.now() - start);
      setTestLatency(duration);
      setTestResponse(JSON.stringify({ error: err instanceof Error ? err.message : 'Request failed' }, null, 2));
    } finally {
      setTestLoading(false);
    }
  };

  return (
    <div className="space-y-6">
      {/* Platform Specification Header */}
      <div className="bg-white p-6 rounded-lg border border-slate-200 shadow-xs">
        <div className="flex items-center space-x-3 pb-4 border-b border-slate-100">
          <div className="p-3 rounded-lg bg-emerald-500/10 text-emerald-600 border border-emerald-500/20">
            <Server className="w-6 h-6" />
          </div>
          <div>
            <h3 className="text-base font-bold text-slate-900">
              Intelligent OS Analytics and Resource Management Platform
            </h3>
            <p className="text-xs text-slate-500">
              Operating Systems Capstone Project • Modular C Backend with React/Vite Telemetry Frontend
            </p>
          </div>
        </div>

        {/* Phase Status Matrix */}
        <div className="grid grid-cols-1 md:grid-cols-4 gap-3 mt-4">
          <div className="p-3 rounded-lg border border-emerald-200 bg-emerald-50/50">
            <div className="flex items-center justify-between text-xs">
              <span className="font-bold text-emerald-800">Phase 1</span>
              <span className="px-1.5 py-0.5 rounded text-[9px] bg-emerald-200 text-emerald-800 font-semibold">Active</span>
            </div>
            <p className="text-xs font-semibold text-slate-800 mt-1">Foundation & Monitoring</p>
            <p className="text-[11px] text-slate-500 mt-0.5">Process table, /proc parsing, CPU/memory telemetry, REST API.</p>
          </div>

          <div className="p-3 rounded-lg border border-slate-200 bg-slate-50">
            <div className="flex items-center justify-between text-xs">
              <span className="font-bold text-slate-700">Phase 2</span>
              <span className="px-1.5 py-0.5 rounded text-[9px] bg-slate-200 text-slate-600 font-semibold">Planned</span>
            </div>
            <p className="text-xs font-semibold text-slate-800 mt-1">Scheduling & Prediction</p>
            <p className="text-[11px] text-slate-500 mt-0.5">FCFS, SJF, SRTF, RR Gantt charts, memory regression, page replacement.</p>
          </div>

          <div className="p-3 rounded-lg border border-slate-200 bg-slate-50">
            <div className="flex items-center justify-between text-xs">
              <span className="font-bold text-slate-700">Phase 3</span>
              <span className="px-1.5 py-0.5 rounded text-[9px] bg-slate-200 text-slate-600 font-semibold">Planned</span>
            </div>
            <p className="text-xs font-semibold text-slate-800 mt-1">Deadlock & Allocation</p>
            <p className="text-[11px] text-slate-500 mt-0.5">Banker's algorithm, system log analysis, explainable resource AI.</p>
          </div>

          <div className="p-3 rounded-lg border border-slate-200 bg-slate-50">
            <div className="flex items-center justify-between text-xs">
              <span className="font-bold text-slate-700">Phase 4</span>
              <span className="px-1.5 py-0.5 rounded text-[9px] bg-slate-200 text-slate-600 font-semibold">Planned</span>
            </div>
            <p className="text-xs font-semibold text-slate-800 mt-1">Integration & Polish</p>
            <p className="text-[11px] text-slate-500 mt-0.5">Unified dashboard, end-to-end testing, architectural documentation.</p>
          </div>
        </div>
      </div>

      {/* C Engine Technical Specifications */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        <div className="bg-white p-5 rounded-lg border border-slate-200 shadow-xs">
          <h4 className="text-xs font-bold text-slate-700 uppercase tracking-wider mb-3 flex items-center space-x-1.5">
            <Code2 className="w-4 h-4 text-emerald-600" />
            <span>C Backend Technical Profile</span>
          </h4>

          <div className="space-y-2 text-xs font-mono">
            <div className="flex justify-between p-2 bg-slate-50 rounded border border-slate-100">
              <span className="text-slate-500">Core Language:</span>
              <span className="font-semibold text-slate-800">C99 / GNU99 (GCC)</span>
            </div>
            <div className="flex justify-between p-2 bg-slate-50 rounded border border-slate-100">
              <span className="text-slate-500">HTTP REST Library:</span>
              <span className="font-semibold text-slate-800">Lightweight Native Socket Engine</span>
            </div>
            <div className="flex justify-between p-2 bg-slate-50 rounded border border-slate-100">
              <span className="text-slate-500">JSON Serialization:</span>
              <span className="font-semibold text-slate-800">Modular Zero-Copy Buffer Builder</span>
            </div>
            <div className="flex justify-between p-2 bg-slate-50 rounded border border-slate-100">
              <span className="text-slate-500">Privilege Level:</span>
              <span className="font-semibold text-emerald-700">Non-Root Safe Execution</span>
            </div>
            <div className="flex justify-between p-2 bg-slate-50 rounded border border-slate-100">
              <span className="text-slate-500">Reported Platform:</span>
              <span className="font-semibold text-slate-800">{health?.platform || 'Detecting...'}</span>
            </div>
          </div>
        </div>

        <div className="bg-white p-5 rounded-lg border border-slate-200 shadow-xs">
          <h4 className="text-xs font-bold text-slate-700 uppercase tracking-wider mb-3 flex items-center space-x-1.5">
            <Terminal className="w-4 h-4 text-blue-600" />
            <span>Linux Virtual Filesystem Interfaces (/proc)</span>
          </h4>

          <div className="space-y-2 text-xs">
            <div className="p-2 bg-slate-50 rounded border border-slate-100 flex items-start space-x-2">
              <FileCode className="w-4 h-4 text-slate-400 shrink-0 mt-0.5" />
              <div>
                <span className="font-mono font-semibold text-slate-800">/proc/stat</span>
                <p className="text-[11px] text-slate-500">Calculates CPU total jiffies, user, nice, system, and idle deltas.</p>
              </div>
            </div>

            <div className="p-2 bg-slate-50 rounded border border-slate-100 flex items-start space-x-2">
              <FileCode className="w-4 h-4 text-slate-400 shrink-0 mt-0.5" />
              <div>
                <span className="font-mono font-semibold text-slate-800">/proc/meminfo</span>
                <p className="text-[11px] text-slate-500">Reads MemTotal, MemFree, MemAvailable, Buffers, Cached, Swap.</p>
              </div>
            </div>

            <div className="p-2 bg-slate-50 rounded border border-slate-100 flex items-start space-x-2">
              <FileCode className="w-4 h-4 text-slate-400 shrink-0 mt-0.5" />
              <div>
                <span className="font-mono font-semibold text-slate-800">/proc/[pid]/stat</span>
                <p className="text-[11px] text-slate-500">Extracts PID, name, state, PPID, utime, stime, priority, threads, RSS, VSZ.</p>
              </div>
            </div>

            <div className="p-2 bg-slate-50 rounded border border-slate-100 flex items-start space-x-2">
              <FileCode className="w-4 h-4 text-slate-400 shrink-0 mt-0.5" />
              <div>
                <span className="font-mono font-semibold text-slate-800">/proc/[pid]/cmdline</span>
                <p className="text-[11px] text-slate-500">Reads null-separated command line invocation arguments.</p>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Live Interactive API Diagnostics Console */}
      <div className="bg-white p-5 rounded-lg border border-slate-200 shadow-xs">
        <div className="flex items-center justify-between mb-3">
          <div>
            <h4 className="text-sm font-bold text-slate-800">Live C API Endpoint Diagnostics</h4>
            <p className="text-xs text-slate-500">Send live HTTP requests directly from the client to the C server</p>
          </div>
          {testLatency !== null && (
            <span className="text-xs font-mono font-semibold text-emerald-600 bg-emerald-50 px-2 py-0.5 rounded border border-emerald-200">
              Latency: {testLatency} ms
            </span>
          )}
        </div>

        {/* Endpoint Selector Buttons */}
        <div className="flex flex-wrap gap-2 mb-4">
          {[
            { label: 'GET /api/health', ep: '/health' },
            { label: 'GET /api/system/overview', ep: '/system/overview' },
            { label: 'GET /api/system/cpu', ep: '/system/cpu' },
            { label: 'GET /api/system/memory', ep: '/system/memory' },
            { label: 'GET /api/processes', ep: '/processes' },
            { label: 'GET /api/processes/4', ep: '/processes/4' },
            { label: 'GET /api/processes/9999999 (404 Test)', ep: '/processes/9999999' },
          ].map((item) => (
            <button
              key={item.ep}
              onClick={() => runEndpointTest(item.ep)}
              disabled={testLoading}
              className={`px-3 py-1.5 rounded-md text-xs font-mono font-medium transition-colors flex items-center space-x-1.5 cursor-pointer ${
                testEndpoint === item.ep
                  ? 'bg-slate-900 text-white shadow-2xs'
                  : 'bg-slate-100 hover:bg-slate-200 text-slate-700'
              }`}
            >
              <Play className="w-2.5 h-2.5" />
              <span>{item.label}</span>
            </button>
          ))}
        </div>

        {/* Live Response Box */}
        <div className="p-3.5 bg-slate-900 text-emerald-400 rounded-lg font-mono text-xs border border-slate-800 max-h-80 overflow-y-auto">
          {testLoading ? (
            <div className="flex items-center space-x-2 text-slate-400 py-4 justify-center">
              <div className="w-4 h-4 border-2 border-emerald-500 border-t-transparent rounded-full animate-spin"></div>
              <span>Querying http://127.0.0.1:8080/api{testEndpoint}...</span>
            </div>
          ) : testResponse ? (
            <pre>{testResponse}</pre>
          ) : (
            <p className="text-slate-500 text-center py-4">Click any endpoint button above to test the C server live.</p>
          )}
        </div>
      </div>
    </div>
  );
};
