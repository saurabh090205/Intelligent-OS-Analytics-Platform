import React, { useEffect, useState } from 'react';
import { X, Terminal, ShieldCheck, AlertCircle } from 'lucide-react';
import type { ProcessDetail } from '../types/system';
import { api } from '../services/api';

interface ProcessModalProps {
  pid: number | null;
  onClose: () => void;
}

export const ProcessModal: React.FC<ProcessModalProps> = ({ pid, onClose }) => {
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [detail, setDetail] = useState<ProcessDetail | null>(null);

  useEffect(() => {
    if (pid === null) return;
    setLoading(true);
    setError(null);

    api.getProcessDetail(pid)
      .then((data) => {
        setDetail(data);
        setLoading(false);
      })
      .catch((err) => {
        setError(err.message || 'Failed to load process details');
        setLoading(false);
      });
  }, [pid]);

  if (pid === null) return null;

  const getStateBadgeClass = (state: string) => {
    switch (state) {
      case 'R': return 'bg-emerald-100 text-emerald-800 border-emerald-300';
      case 'S': return 'bg-blue-100 text-blue-800 border-blue-300';
      case 'D': return 'bg-amber-100 text-amber-800 border-amber-300';
      case 'Z': return 'bg-rose-100 text-rose-800 border-rose-300';
      case 'T': return 'bg-orange-100 text-orange-800 border-orange-300';
      default:  return 'bg-slate-100 text-slate-800 border-slate-300';
    }
  };

  return (
    <div className="fixed inset-0 z-50 bg-slate-900/50 backdrop-blur-xs flex items-center justify-center p-4">
      <div className="bg-white rounded-xl shadow-2xl border border-slate-200 max-w-2xl w-full overflow-hidden animate-in fade-in zoom-in-95 duration-150">
        {/* Modal Header */}
        <div className="px-6 py-4 bg-slate-900 text-white flex items-center justify-between">
          <div className="flex items-center space-x-3">
            <div className="p-2 rounded-lg bg-emerald-500/20 text-emerald-400">
              <Terminal className="w-5 h-5" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h3 className="text-base font-bold tracking-tight">Process #{pid}</h3>
                {detail && (
                  <span className={`text-[10px] px-2 py-0.5 rounded font-semibold border ${getStateBadgeClass(detail.state)}`}>
                    {detail.state_full} ({detail.state})
                  </span>
                )}
              </div>
              <p className="text-xs text-slate-400 font-mono truncate max-w-md">
                {detail ? detail.name : 'Loading metadata...'}
              </p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="p-1 rounded-md text-slate-400 hover:text-white hover:bg-slate-800 transition-colors cursor-pointer"
          >
            <X className="w-5 h-5" />
          </button>
        </div>

        {/* Modal Body */}
        <div className="p-6">
          {loading && (
            <div className="py-12 text-center">
              <div className="inline-block w-8 h-8 border-3 border-emerald-600 border-t-transparent rounded-full animate-spin"></div>
              <p className="mt-3 text-xs text-slate-500 font-medium">Sampling process telemetry from system /proc...</p>
            </div>
          )}

          {error && (
            <div className="py-8 text-center">
              <AlertCircle className="w-10 h-10 text-rose-500 mx-auto mb-2" />
              <h4 className="text-sm font-semibold text-slate-800">Process Query Failed</h4>
              <p className="text-xs text-slate-500 mt-1 max-w-md mx-auto">{error}</p>
              <button
                onClick={onClose}
                className="mt-4 px-4 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-medium rounded-md transition-colors"
              >
                Close
              </button>
            </div>
          )}

          {detail && !loading && (
            <div className="space-y-5">
              {/* Command Line / Binary Path */}
              <div>
                <label className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider block mb-1">
                  Command Line / Executable Path
                </label>
                <div className="p-2.5 bg-slate-900 text-emerald-400 rounded-md font-mono text-xs break-all border border-slate-800 max-h-24 overflow-y-auto">
                  {detail.cmdline || `[${detail.name}]`}
                </div>
              </div>

              {/* Grid Metrics */}
              <div className="grid grid-cols-2 sm:grid-cols-4 gap-3">
                <div className="p-3 bg-slate-50 rounded-lg border border-slate-200">
                  <span className="text-[10px] font-medium text-slate-500 block uppercase">CPU Usage</span>
                  <span className="text-lg font-bold font-mono text-slate-900 mt-0.5 block">
                    {detail.cpu_pct.toFixed(2)}%
                  </span>
                </div>

                <div className="p-3 bg-slate-50 rounded-lg border border-slate-200">
                  <span className="text-[10px] font-medium text-slate-500 block uppercase">Physical Memory (RSS)</span>
                  <span className="text-lg font-bold font-mono text-slate-900 mt-0.5 block">
                    {(detail.mem_rss_kb / 1024).toFixed(1)} MB
                  </span>
                  <span className="text-[10px] text-slate-400 font-mono">({detail.mem_pct.toFixed(2)}% RAM)</span>
                </div>

                <div className="p-3 bg-slate-50 rounded-lg border border-slate-200">
                  <span className="text-[10px] font-medium text-slate-500 block uppercase">Virtual Memory (VSZ)</span>
                  <span className="text-lg font-bold font-mono text-slate-900 mt-0.5 block">
                    {(detail.mem_vsz_kb / 1024).toFixed(1)} MB
                  </span>
                </div>

                <div className="p-3 bg-slate-50 rounded-lg border border-slate-200">
                  <span className="text-[10px] font-medium text-slate-500 block uppercase">Thread Count</span>
                  <span className="text-lg font-bold font-mono text-slate-900 mt-0.5 block">
                    {detail.threads}
                  </span>
                </div>
              </div>

              {/* Kernel Sched & Process Hierarchy */}
              <div className="grid grid-cols-2 gap-3 text-xs">
                <div className="p-3 bg-slate-50 rounded-lg border border-slate-200 space-y-1.5 font-mono">
                  <div className="text-[11px] font-sans font-semibold text-slate-700 uppercase mb-2">Process Hierarchy</div>
                  <div className="flex justify-between text-slate-600">
                    <span>Process ID (PID):</span>
                    <span className="font-semibold text-slate-900">{detail.pid}</span>
                  </div>
                  <div className="flex justify-between text-slate-600">
                    <span>Parent PID (PPID):</span>
                    <span className="font-semibold text-slate-900">{detail.ppid}</span>
                  </div>
                  <div className="flex justify-between text-slate-600">
                    <span>Process Name:</span>
                    <span className="font-semibold text-slate-900">{detail.name}</span>
                  </div>
                </div>

                <div className="p-3 bg-slate-50 rounded-lg border border-slate-200 space-y-1.5 font-mono">
                  <div className="text-[11px] font-sans font-semibold text-slate-700 uppercase mb-2">Scheduler Stats</div>
                  <div className="flex justify-between text-slate-600">
                    <span>Priority (PR):</span>
                    <span className="font-semibold text-slate-900">{detail.priority}</span>
                  </div>
                  <div className="flex justify-between text-slate-600">
                    <span>Nice Level (NI):</span>
                    <span className="font-semibold text-slate-900">{detail.nice}</span>
                  </div>
                  <div className="flex justify-between text-slate-600">
                    <span>User CPU Ticks:</span>
                    <span className="font-semibold text-slate-900">{detail.utime_ticks}</span>
                  </div>
                  <div className="flex justify-between text-slate-600">
                    <span>Kernel CPU Ticks:</span>
                    <span className="font-semibold text-slate-900">{detail.stime_ticks}</span>
                  </div>
                </div>
              </div>

              {/* Info Note */}
              <div className="p-3 bg-emerald-50/70 border border-emerald-200 rounded-md text-[11px] text-emerald-900 flex items-start space-x-2">
                <ShieldCheck className="w-4 h-4 text-emerald-600 shrink-0 mt-0.5" />
                <span>
                  Telemetry sampled cleanly without elevated root privileges from the Linux <code>/proc/{pid}/stat</code> and <code>/proc/{pid}/cmdline</code> virtual filesystem interfaces.
                </span>
              </div>
            </div>
          )}
        </div>

        {/* Modal Footer */}
        <div className="px-6 py-3 bg-slate-50 border-t border-slate-200 flex justify-end">
          <button
            onClick={onClose}
            className="px-4 py-1.5 bg-slate-800 hover:bg-slate-900 text-white text-xs font-medium rounded-md transition-colors cursor-pointer"
          >
            Done
          </button>
        </div>
      </div>
    </div>
  );
};
