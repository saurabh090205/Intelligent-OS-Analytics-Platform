import React, { useState, useEffect, useCallback } from 'react';
import {
  FileText,
  Search,
  AlertTriangle,
  AlertOctagon,
  Sparkles,
  Zap,
  Terminal,
  RefreshCw,
  Layers
} from 'lucide-react';
import { api } from '../services/api';
import type { LogAnalysisResponse } from '../types/system';

const SAMPLE_LOGS: Record<string, { title: string; desc: string; text: string }> = {
  linux_syslog: {
    title: 'Linux Syslog (OOM Killer & Segfault)',
    desc: 'RFC 3164 BSD timestamp logs with Out-Of-Memory kill events and systemd core-dump crashes.',
    text: `Oct 25 14:10:01 host-srv systemd[1]: Started Daily apt upgrade and clean activities.
Oct 25 14:10:05 host-srv sshd[1420]: Failed password for invalid user admin from 192.168.1.105 port 42890 ssh2
Oct 25 14:10:07 host-srv sshd[1420]: Failed password for invalid user admin from 192.168.1.105 port 42890 ssh2
Oct 25 14:11:15 host-srv kernel: [ 4210.124] EXT4-fs (sda1): warning: mounting unchecked fs, running e2fsck is recommended
Oct 25 14:12:44 host-srv kernel: [ 4299.882] Out of memory: Kill process 8912 (stress-ng-vm) score 842 or sacrifice child
Oct 25 14:12:44 host-srv kernel: [ 4299.885] oom-killer: gfp_mask=0x100cca(GFP_HIGHUSER_MOVABLE), order=0, oom_score_adj=0
Oct 25 14:12:45 host-srv kernel: [ 4300.012] Killed process 8912 (stress-ng-vm) total-vm:4289120kB, anon-rss:3910240kB, file-rss:0kB
Oct 25 14:14:02 host-srv systemd[1]: nginx.service: Main process exited, code=dumped, status=11/SEGV
Oct 25 14:14:02 host-srv systemd[1]: nginx.service: Failed with result 'core-dump'.
Oct 25 14:15:20 host-srv kernel: [ 4455.109] traps: worker[9104] general protection fault ip:7f890124 sp:7ffc1290 error:0
Oct 25 14:16:30 host-srv postfix/pickup[4910]: fatal: open /etc/postfix/main.cf: No space left on device
Oct 25 14:17:00 host-srv CRON[9200]: (root) CMD (   cd / && run-parts --report /etc/cron.hourly)`
  },
  windows_events: {
    title: 'Windows Event Logs (Storage & Service)',
    desc: 'Structured Windows application and system logs with NTFS warnings and memory page allocations.',
    text: `Level: Information, Date: 2026-10-25 14:05:12, Source: Service Control Manager, Message: The Windows Update service entered the running state.
Level: Warning, Date: 2026-10-25 14:08:44, Source: Ntfs, Message: Volume C: the file system structure on disk is corrupt and unusable. Please run chkdsk.
Level: Error, Date: 2026-10-25 14:11:30, Source: Application Error, Message: Faulting application name: sqlservr.exe, version: 15.0.2000, faulting module: ntdll.dll, exception code: 0xc0000005.
Level: Critical, Date: 2026-10-25 14:13:00, Source: Microsoft-Windows-Kernel-Memory, Message: The system committed memory reached limit. Allocation of 209715200 bytes failed.
Level: Warning, Date: 2026-10-25 14:15:22, Source: TCPIP, Message: TCP/IP failed to establish an outgoing connection because the selected local endpoint was already in use.
Level: Information, Date: 2026-10-25 14:16:40, Source: EventLog, Message: The system uptime is 284102 seconds.`
  },
  kernel_burst: {
    title: 'Kernel Crash Burst (Rapid Cascading Errors)',
    desc: 'Simulated high-frequency error stream triggering real-time burst rate anomaly alarms.',
    text: `2026-10-25T14:20:01Z host-srv kernel: [ 5001.001] Kernel panic - not syncing: Fatal exception in interrupt
2026-10-25T14:20:02Z host-srv kernel: [ 5001.002] Kernel panic - not syncing: Fatal exception in interrupt
2026-10-25T14:20:03Z host-srv kernel: [ 5001.003] Kernel panic - not syncing: Fatal exception in interrupt
2026-10-25T14:20:04Z host-srv kernel: [ 5001.004] Out of memory: Kill process 1240
2026-10-25T14:20:05Z host-srv kernel: [ 5001.005] Kernel panic - not syncing: Fatal exception in interrupt
2026-10-25T14:20:06Z host-srv kernel: [ 5001.006] Out of memory: Kill process 1241`
  }
};

export const LogAnalysisPage: React.FC = () => {
  const [logText, setLogText] = useState<string>(SAMPLE_LOGS.linux_syslog.text);
  const [filePath, setFilePath] = useState<string>('');
  const [inputMode, setInputMode] = useState<'text' | 'file'>('text');

  // Filter controls
  const [minSeverity, setMinSeverity] = useState<string>('ALL');
  const [categoryFilter, setCategoryFilter] = useState<number>(-1);
  const [keyword, setKeyword] = useState<string>('');

  // Results
  const [report, setReport] = useState<LogAnalysisResponse | null>(null);
  const [isLoading, setIsLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);

  const analyze = useCallback(async (
    text: string,
    file: string,
    mode: 'text' | 'file',
    sev: string,
    cat: number,
    kw: string
  ) => {
    setIsLoading(true);
    setError(null);
    try {
      const payload: {
        content?: string;
        file_path?: string;
        min_severity?: string;
        category?: number;
        keyword?: string;
      } = {
        min_severity: sev,
        category: cat,
        keyword: kw,
      };

      if (mode === 'file') {
        if (!file.trim()) {
          setError('Please provide an absolute or relative path to a local log file.');
          setIsLoading(false);
          return;
        }
        payload.file_path = file.trim();
      } else {
        if (!text.trim()) {
          setError('Please paste or enter log content to analyze.');
          setIsLoading(false);
          return;
        }
        payload.content = text;
      }

      const res = await api.analyzeLogs(payload);
      setReport(res);
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Log analysis failed');
    } finally {
      setIsLoading(false);
    }
  }, []);

  // Initial analysis on load
  useEffect(() => {
    analyze(logText, filePath, inputMode, minSeverity, categoryFilter, keyword);
  }, []);

  const handleSelectPreset = (key: string) => {
    const preset = SAMPLE_LOGS[key];
    if (preset) {
      setInputMode('text');
      setLogText(preset.text);
      analyze(preset.text, '', 'text', minSeverity, categoryFilter, keyword);
    }
  };

  const getSeverityBadgeClass = (sev: string) => {
    switch (sev.toUpperCase()) {
      case 'EMERGENCY':
      case 'ALERT':
      case 'CRITICAL':
        return 'bg-rose-100 text-rose-800 border-rose-200';
      case 'ERROR':
        return 'bg-amber-100 text-amber-800 border-amber-200';
      case 'WARNING':
        return 'bg-yellow-100 text-yellow-800 border-yellow-200';
      case 'NOTICE':
      case 'INFO':
        return 'bg-sky-100 text-sky-800 border-sky-200';
      case 'DEBUG':
        return 'bg-slate-100 text-slate-700 border-slate-200';
      default:
        return 'bg-slate-100 text-slate-600 border-slate-200';
    }
  };

  return (
    <div className="space-y-6 pb-12">
      {/* Page Header */}
      <div className="bg-white border border-slate-200 rounded-xl p-6 shadow-xs flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center space-x-3 mb-1">
            <div className="p-2 bg-indigo-50 text-indigo-600 rounded-lg">
              <FileText className="w-5 h-5" />
            </div>
            <h1 className="text-xl font-bold text-slate-800 tracking-tight">
              Operating System Log Analysis
            </h1>
            <span className="px-2.5 py-0.5 rounded-full text-xs font-semibold bg-indigo-100 text-indigo-700">
              Module 6 (Phase 3)
            </span>
          </div>
          <p className="text-sm text-slate-500">
            Multi-format parser for Linux Syslog (RFC 3164/5424), ISO 8601, and Windows Event logs with error categorizer &amp; burst detection.
          </p>
        </div>

        <div className="flex items-center space-x-3">
          <button
            onClick={() => analyze(logText, filePath, inputMode, minSeverity, categoryFilter, keyword)}
            disabled={isLoading}
            className="flex items-center space-x-2 px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white font-medium rounded-lg shadow-xs transition-colors cursor-pointer text-sm disabled:opacity-50"
          >
            <RefreshCw className={`w-4 h-4 ${isLoading ? 'animate-spin' : ''}`} />
            <span>{isLoading ? 'Parsing Logs...' : 'Analyze Log Stream'}</span>
          </button>
        </div>
      </div>

      {/* Preset Log Selectors */}
      <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
        <div className="flex items-center space-x-2 mb-3">
          <Sparkles className="w-4 h-4 text-indigo-600" />
          <span className="text-xs font-bold uppercase tracking-wider text-slate-500">
            Representative Operating System Log Samples
          </span>
        </div>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
          {Object.entries(SAMPLE_LOGS).map(([key, item]) => (
            <button
              key={key}
              onClick={() => handleSelectPreset(key)}
              className="p-3 text-left rounded-lg border border-slate-200 hover:border-indigo-400 bg-slate-50/50 hover:bg-indigo-50/30 transition-all cursor-pointer"
            >
              <div className="flex items-center justify-between mb-1">
                <span className="text-xs font-bold text-slate-800">{item.title}</span>
                <span className="text-[10px] bg-slate-200 text-slate-700 px-1.5 py-0.5 rounded font-mono">
                  {key}
                </span>
              </div>
              <p className="text-[11px] text-slate-500 line-clamp-2 leading-relaxed">
                {item.desc}
              </p>
            </button>
          ))}
        </div>
      </div>

      {/* Input Mode & Payload Configurator */}
      <div className="bg-white border border-slate-200 rounded-xl p-6 shadow-xs space-y-4">
        <div className="flex items-center justify-between border-b border-slate-200 pb-3">
          <div className="flex items-center space-x-4">
            <button
              onClick={() => setInputMode('text')}
              className={`text-xs font-bold uppercase tracking-wider pb-1 transition-colors cursor-pointer ${
                inputMode === 'text'
                  ? 'text-indigo-600 border-b-2 border-indigo-600'
                  : 'text-slate-400 hover:text-slate-600'
              }`}
            >
              Direct Log Text Stream
            </button>
            <button
              onClick={() => setInputMode('file')}
              className={`text-xs font-bold uppercase tracking-wider pb-1 transition-colors cursor-pointer ${
                inputMode === 'file'
                  ? 'text-indigo-600 border-b-2 border-indigo-600'
                  : 'text-slate-400 hover:text-slate-600'
              }`}
            >
              Local Host Log File Path
            </button>
          </div>
          <span className="text-xs text-slate-400">
            {inputMode === 'text' ? `${logText.split('\n').filter(Boolean).length} lines` : 'Local file'}
          </span>
        </div>

        {inputMode === 'text' ? (
          <div>
            <textarea
              rows={6}
              value={logText}
              onChange={(e) => setLogText(e.target.value)}
              placeholder="Paste raw operating system log lines here..."
              className="w-full text-xs font-mono bg-slate-50 border border-slate-300 rounded-lg p-3 text-slate-800 focus:ring-1 focus:ring-indigo-500 outline-hidden resize-y"
            />
          </div>
        ) : (
          <div>
            <label className="text-xs font-bold text-slate-600 uppercase tracking-wider block mb-1">
              Absolute File Path
            </label>
            <input
              type="text"
              value={filePath}
              onChange={(e) => setFilePath(e.target.value)}
              placeholder="e.g., C:\logs\system.log or d:\OS CP\backend\tests\sample.log"
              className="w-full text-xs font-mono bg-slate-50 border border-slate-300 rounded-lg p-2.5 text-slate-800 focus:ring-1 focus:ring-indigo-500 outline-hidden"
            />
            <p className="text-[11px] text-slate-400 mt-1">
              Backend validates path existence, reads in chunks safely, and skips unreadable lines without memory overflow.
            </p>
          </div>
        )}

        {/* Filter Controls Bar */}
        <div className="grid grid-cols-1 sm:grid-cols-3 gap-3 pt-2">
          {/* Severity Filter */}
          <div>
            <label className="text-[11px] font-semibold text-slate-500 uppercase tracking-wider block mb-1">
              Minimum Severity
            </label>
            <select
              value={minSeverity}
              onChange={(e) => {
                const val = e.target.value;
                setMinSeverity(val);
                analyze(logText, filePath, inputMode, val, categoryFilter, keyword);
              }}
              className="w-full border border-slate-300 rounded-lg px-3 py-1.5 text-xs font-semibold bg-white focus:ring-1 focus:ring-indigo-500 outline-hidden"
            >
              <option value="ALL">ALL (Include Debug &amp; Info)</option>
              <option value="INFO">INFO and higher</option>
              <option value="WARNING">WARNING and higher</option>
              <option value="ERROR">ERROR and higher</option>
              <option value="CRITICAL">CRITICAL only</option>
            </select>
          </div>

          {/* Category Filter */}
          <div>
            <label className="text-[11px] font-semibold text-slate-500 uppercase tracking-wider block mb-1">
              Category
            </label>
            <select
              value={categoryFilter}
              onChange={(e) => {
                const val = parseInt(e.target.value, 10);
                setCategoryFilter(val);
                analyze(logText, filePath, inputMode, minSeverity, val, keyword);
              }}
              className="w-full border border-slate-300 rounded-lg px-3 py-1.5 text-xs font-semibold bg-white focus:ring-1 focus:ring-indigo-500 outline-hidden"
            >
              <option value={-1}>All Categories</option>
              <option value={1}>Memory / OOM</option>
              <option value={2}>Kernel Panic / Crash</option>
              <option value={3}>Storage &amp; Disk</option>
              <option value={4}>Auth &amp; Security</option>
              <option value={5}>Network</option>
              <option value={6}>Service Crash</option>
              <option value={0}>General OS</option>
            </select>
          </div>

          {/* Keyword Search */}
          <div>
            <label className="text-[11px] font-semibold text-slate-500 uppercase tracking-wider block mb-1">
              Keyword Filter
            </label>
            <div className="relative">
              <input
                type="text"
                value={keyword}
                onChange={(e) => setKeyword(e.target.value)}
                onKeyDown={(e) => {
                  if (e.key === 'Enter') {
                    analyze(logText, filePath, inputMode, minSeverity, categoryFilter, keyword);
                  }
                }}
                placeholder="Search message text..."
                className="w-full border border-slate-300 rounded-lg pl-8 pr-3 py-1.5 text-xs bg-white focus:ring-1 focus:ring-indigo-500 outline-hidden"
              />
              <Search className="w-3.5 h-3.5 text-slate-400 absolute left-2.5 top-2.5" />
            </div>
          </div>
        </div>
      </div>

      {/* Error Alert */}
      {error && (
        <div className="bg-rose-50 border border-rose-200 rounded-xl p-4 text-rose-700 text-sm flex items-center space-x-2">
          <AlertTriangle className="w-5 h-5 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      {/* Error Burst Detection Banner */}
      {report?.burst_detected && (
        <div className="bg-rose-600 text-white rounded-xl p-4 shadow-sm flex items-center space-x-3">
          <Zap className="w-6 h-6 shrink-0 animate-pulse" />
          <div>
            <h4 className="font-bold text-sm">Critical Log Anomaly: Error Burst Detected</h4>
            <p className="text-xs text-rose-100 mt-0.5">{report.burst_description}</p>
          </div>
        </div>
      )}

      {/* KPI Overview Metrics */}
      <div className="grid grid-cols-2 md:grid-cols-5 gap-4">
        <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
          <span className="text-[11px] font-semibold text-slate-500 uppercase tracking-wider block mb-1">
            Total Lines
          </span>
          <div className="text-2xl font-bold text-slate-800">
            {report?.total_lines_read ?? 0}
          </div>
          <span className="text-[11px] text-slate-400">Stream input lines</span>
        </div>

        <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
          <span className="text-[11px] font-semibold text-rose-600 uppercase tracking-wider block mb-1">
            Errors &amp; Panics
          </span>
          <div className="text-2xl font-bold text-rose-600">
            {report?.error_count ?? 0}
          </div>
          <span className="text-[11px] text-rose-500 font-medium">Critical attention</span>
        </div>

        <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
          <span className="text-[11px] font-semibold text-amber-600 uppercase tracking-wider block mb-1">
            Warnings
          </span>
          <div className="text-2xl font-bold text-amber-600">
            {report?.warning_count ?? 0}
          </div>
          <span className="text-[11px] text-amber-500 font-medium">Subsystem alerts</span>
        </div>

        <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
          <span className="text-[11px] font-semibold text-sky-600 uppercase tracking-wider block mb-1">
            Info &amp; Debug
          </span>
          <div className="text-2xl font-bold text-sky-600">
            {(report?.info_count ?? 0) + (report?.debug_count ?? 0)}
          </div>
          <span className="text-[11px] text-slate-400">Routine state logs</span>
        </div>

        <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
          <span className="text-[11px] font-semibold text-slate-500 uppercase tracking-wider block mb-1">
            Unrecognized
          </span>
          <div className="text-2xl font-bold text-slate-700">
            {report?.unrecognized_lines_count ?? 0}
          </div>
          <span className="text-[11px] text-slate-400">Freeform / unformatted</span>
        </div>
      </div>

      {/* Recurring Patterns and Category Breakdown */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {/* Recurring Error Patterns */}
        <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-xs">
          <div className="flex items-center space-x-2 mb-3">
            <AlertOctagon className="w-4 h-4 text-indigo-600" />
            <h3 className="text-sm font-bold text-slate-800 uppercase tracking-wide">
              Recurring Error &amp; Failure Patterns
            </h3>
          </div>
          {report?.recurring_patterns && report.recurring_patterns.length > 0 ? (
            <div className="space-y-2">
              {report.recurring_patterns.map((pat, idx) => (
                <div
                  key={idx}
                  className="flex items-center justify-between p-2.5 bg-slate-50 rounded-lg border border-slate-200"
                >
                  <div className="flex items-center space-x-2">
                    <span className={`px-2 py-0.5 rounded text-[10px] font-bold border ${getSeverityBadgeClass(pat.severity)}`}>
                      {pat.severity}
                    </span>
                    <span className="text-xs font-semibold text-slate-700">{pat.pattern}</span>
                  </div>
                  <span className="px-2 py-0.5 bg-slate-200 text-slate-800 rounded font-mono text-xs font-bold">
                    {pat.count} &times;
                  </span>
                </div>
              ))}
            </div>
          ) : (
            <div className="p-4 bg-slate-50 rounded-lg text-center text-xs text-slate-500">
              No recurring high-severity patterns identified in current log slice.
            </div>
          )}
        </div>

        {/* Subsystem Category Distribution */}
        <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-xs">
          <div className="flex items-center space-x-2 mb-3">
            <Layers className="w-4 h-4 text-indigo-600" />
            <h3 className="text-sm font-bold text-slate-800 uppercase tracking-wide">
              Subsystem Category Distribution
            </h3>
          </div>
          {report?.category_counts ? (
            <div className="grid grid-cols-2 gap-2 text-xs">
              <div className="p-2.5 bg-slate-50 rounded border border-slate-200 flex justify-between items-center">
                <span className="text-slate-600 font-medium">Memory / OOM:</span>
                <span className="font-mono font-bold text-indigo-600">{report.category_counts.memory_oom}</span>
              </div>
              <div className="p-2.5 bg-slate-50 rounded border border-slate-200 flex justify-between items-center">
                <span className="text-slate-600 font-medium">Kernel Panic:</span>
                <span className="font-mono font-bold text-rose-600">{report.category_counts.kernel_panic}</span>
              </div>
              <div className="p-2.5 bg-slate-50 rounded border border-slate-200 flex justify-between items-center">
                <span className="text-slate-600 font-medium">Storage &amp; Disk:</span>
                <span className="font-mono font-bold text-amber-600">{report.category_counts.storage_disk}</span>
              </div>
              <div className="p-2.5 bg-slate-50 rounded border border-slate-200 flex justify-between items-center">
                <span className="text-slate-600 font-medium">Auth &amp; Security:</span>
                <span className="font-mono font-bold text-purple-600">{report.category_counts.auth_security}</span>
              </div>
              <div className="p-2.5 bg-slate-50 rounded border border-slate-200 flex justify-between items-center">
                <span className="text-slate-600 font-medium">Network:</span>
                <span className="font-mono font-bold text-sky-600">{report.category_counts.network}</span>
              </div>
              <div className="p-2.5 bg-slate-50 rounded border border-slate-200 flex justify-between items-center">
                <span className="text-slate-600 font-medium">Service Crash:</span>
                <span className="font-mono font-bold text-red-600">{report.category_counts.service_crash}</span>
              </div>
            </div>
          ) : null}
        </div>
      </div>

      {/* Filtered Parsed Log Entries Table */}
      <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-xs space-y-4">
        <div className="flex items-center justify-between">
          <div className="flex items-center space-x-2">
            <Terminal className="w-4 h-4 text-indigo-600" />
            <h3 className="text-sm font-bold text-slate-800 uppercase tracking-wide">
              Parsed Event Log Stream ({report?.entries?.length ?? 0} matching entries)
            </h3>
          </div>
          <span className="text-xs text-slate-400">
            {report?.unrecognized_lines_count ?? 0} unformatted lines handled safely
          </span>
        </div>

        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs border-collapse">
            <thead>
              <tr className="border-b border-slate-200 text-slate-500 font-semibold bg-slate-50/70">
                <th className="py-2 px-2.5 w-12">#</th>
                <th className="py-2 px-3 w-36">Timestamp</th>
                <th className="py-2 px-2.5 w-24">Severity</th>
                <th className="py-2 px-3 w-32">Category</th>
                <th className="py-2 px-2.5 w-28">Source</th>
                <th className="py-2 px-3">Message</th>
                <th className="py-2 px-2 w-20 text-center">Format</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-100 font-mono text-[11px]">
              {report?.entries && report.entries.length > 0 ? (
                report.entries.map((ent, idx) => (
                  <tr key={idx} className="hover:bg-slate-50/60">
                    <td className="py-2 px-2.5 text-slate-400">{ent.line}</td>
                    <td className="py-2 px-3 text-slate-600 whitespace-nowrap">{ent.timestamp}</td>
                    <td className="py-2 px-2.5">
                      <span className={`px-2 py-0.5 rounded text-[10px] font-bold border ${getSeverityBadgeClass(ent.severity)}`}>
                        {ent.severity}
                      </span>
                    </td>
                    <td className="py-2 px-3 font-sans text-slate-700 whitespace-nowrap">
                      {ent.category}
                    </td>
                    <td className="py-2 px-2.5 text-slate-500 whitespace-nowrap font-bold">
                      {ent.source || '-'}
                    </td>
                    <td className="py-2 px-3 text-slate-800 font-mono break-all font-medium">
                      {ent.message}
                    </td>
                    <td className="py-2 px-2 text-center">
                      {ent.is_supported_format ? (
                        <span className="px-1.5 py-0.5 bg-emerald-50 text-emerald-700 rounded text-[9px] font-sans font-bold border border-emerald-200">
                          Structured
                        </span>
                      ) : (
                        <span className="px-1.5 py-0.5 bg-slate-100 text-slate-500 rounded text-[9px] font-sans">
                          Freeform
                        </span>
                      )}
                    </td>
                  </tr>
                ))
              ) : (
                <tr>
                  <td colSpan={7} className="py-8 text-center text-slate-400 font-sans text-xs">
                    No log events matching the selected filters.
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
};
