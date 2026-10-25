import React, { useState, useMemo } from 'react';
import {
  Terminal,
  Search,
  Filter,
  ArrowUpDown,
  ArrowUp,
  ArrowDown,
  Info,
  ShieldCheck,
  ChevronLeft,
  ChevronRight,
  RefreshCw,
  ExternalLink
} from 'lucide-react';
import type { ProcessSummary } from '../types/system';

interface ProcessMonitorPageProps {
  processes: ProcessSummary[];
  totalProcesses: number;
  onInspectProcess: (pid: number) => void;
  onManualRefresh: () => void;
  isRefreshing: boolean;
  searchQuery: string;
  onSearchChange: (q: string) => void;
}

type SortField = 'pid' | 'name' | 'state' | 'cpu_pct' | 'mem_kb' | 'mem_pct' | 'threads' | 'ppid';
type SortDirection = 'asc' | 'desc';

export const ProcessMonitorPage: React.FC<ProcessMonitorPageProps> = ({
  processes,
  totalProcesses,
  onInspectProcess,
  onManualRefresh,
  isRefreshing,
  searchQuery,
  onSearchChange,
}) => {
  const [selectedStateFilter, setSelectedStateFilter] = useState<string>('ALL');
  const [sortField, setSortField] = useState<SortField>('cpu_pct');
  const [sortDir, setSortDir] = useState<SortDirection>('desc');
  const [currentPage, setCurrentPage] = useState<number>(1);
  const [pageSize, setPageSize] = useState<number>(25);

  const handleSort = (field: SortField) => {
    if (sortField === field) {
      setSortDir(sortDir === 'asc' ? 'desc' : 'asc');
    } else {
      setSortField(field);
      setSortDir('desc');
    }
    setCurrentPage(1);
  };

  // Filter processes based on search query and state
  const filteredProcesses = useMemo(() => {
    return processes.filter((proc) => {
      // State filter
      if (selectedStateFilter !== 'ALL') {
        if (selectedStateFilter === 'RUNNING' && proc.state !== 'R') return false;
        if (selectedStateFilter === 'SLEEPING' && proc.state !== 'S' && proc.state !== 'D' && proc.state !== 'I') return false;
        if (selectedStateFilter === 'STOPPED' && proc.state !== 'T' && proc.state !== 't') return false;
        if (selectedStateFilter === 'ZOMBIE' && proc.state !== 'Z') return false;
      }

      // Search query
      if (searchQuery.trim()) {
        const query = searchQuery.toLowerCase().trim();
        const matchesName = proc.name.toLowerCase().includes(query);
        const matchesPid = proc.pid.toString().includes(query);
        const matchesPpid = proc.ppid.toString().includes(query);
        return matchesName || matchesPid || matchesPpid;
      }

      return true;
    });
  }, [processes, selectedStateFilter, searchQuery]);

  // Sort processes
  const sortedProcesses = useMemo(() => {
    return [...filteredProcesses].sort((a, b) => {
      let valA = a[sortField];
      let valB = b[sortField];

      if (typeof valA === 'string') {
        valA = (valA as string).toLowerCase();
        valB = (valB as string).toLowerCase();
      }

      if (valA < valB) return sortDir === 'asc' ? -1 : 1;
      if (valA > valB) return sortDir === 'asc' ? 1 : -1;
      return 0;
    });
  }, [filteredProcesses, sortField, sortDir]);

  // Pagination
  const totalPages = Math.ceil(sortedProcesses.length / pageSize) || 1;
  const paginatedProcesses = useMemo(() => {
    const start = (currentPage - 1) * pageSize;
    return sortedProcesses.slice(start, start + pageSize);
  }, [sortedProcesses, currentPage, pageSize]);

  const getStateBadge = (state: string, stateFull: string) => {
    switch (state) {
      case 'R':
        return <span className="px-2 py-0.5 rounded text-[10px] font-semibold bg-emerald-100 text-emerald-800 border border-emerald-300">Running</span>;
      case 'S':
        return <span className="px-2 py-0.5 rounded text-[10px] font-semibold bg-blue-100 text-blue-800 border border-blue-200">Sleeping</span>;
      case 'D':
        return <span className="px-2 py-0.5 rounded text-[10px] font-semibold bg-amber-100 text-amber-800 border border-amber-300">Disk Sleep</span>;
      case 'Z':
        return <span className="px-2 py-0.5 rounded text-[10px] font-semibold bg-rose-100 text-rose-800 border border-rose-300">Zombie</span>;
      case 'T':
      case 't':
        return <span className="px-2 py-0.5 rounded text-[10px] font-semibold bg-orange-100 text-orange-800 border border-orange-300">Stopped</span>;
      default:
        return <span className="px-2 py-0.5 rounded text-[10px] font-semibold bg-slate-100 text-slate-700 border border-slate-300">{stateFull || state}</span>;
    }
  };

  const renderSortIcon = (field: SortField) => {
    if (sortField !== field) {
      return <ArrowUpDown className="w-3 h-3 text-slate-300 opacity-0 group-hover:opacity-100 transition-opacity" />;
    }
    return sortDir === 'asc' ? (
      <ArrowUp className="w-3 h-3 text-emerald-600 font-bold" />
    ) : (
      <ArrowDown className="w-3 h-3 text-emerald-600 font-bold" />
    );
  };

  return (
    <div className="space-y-4">
      {/* Informative Notice Banner */}
      <div className="p-3 bg-white border border-slate-200 rounded-lg shadow-xs flex items-center justify-between text-xs">
        <div className="flex items-center space-x-2 text-slate-700">
          <ShieldCheck className="w-4 h-4 text-emerald-600 shrink-0" />
          <span>
            <strong>Actual Accessible Operating System Processes</strong>: Displaying {processes.length} accessible processes sampled directly from system interfaces (<code>/proc</code>). Privileged root-only kernel entries are safely bypassed.
          </span>
        </div>
        <div className="flex items-center space-x-2 shrink-0">
          <span className="text-[11px] text-slate-500 font-mono font-medium">
            Total System Tasks: {totalProcesses}
          </span>
          <button
            onClick={onManualRefresh}
            disabled={isRefreshing}
            className="p-1 rounded text-slate-400 hover:text-emerald-600 hover:bg-slate-100 transition-colors cursor-pointer"
            title="Resample process table"
          >
            <RefreshCw className={`w-3.5 h-3.5 ${isRefreshing ? 'animate-spin' : ''}`} />
          </button>
        </div>
      </div>

      {/* Filter and Search Controls Bar */}
      <div className="bg-white p-3.5 rounded-lg border border-slate-200 shadow-xs flex flex-wrap items-center justify-between gap-3">
        {/* State Filter Pills */}
        <div className="flex items-center space-x-1.5 text-xs">
          <span className="text-slate-400 text-[11px] mr-1 flex items-center space-x-1">
            <Filter className="w-3 h-3" />
            <span>State:</span>
          </span>

          {(['ALL', 'RUNNING', 'SLEEPING', 'STOPPED', 'ZOMBIE'] as const).map((filterKey) => (
            <button
              key={filterKey}
              onClick={() => { setSelectedStateFilter(filterKey); setCurrentPage(1); }}
              className={`px-2.5 py-1 rounded-md text-xs font-medium transition-colors cursor-pointer ${
                selectedStateFilter === filterKey
                  ? 'bg-slate-900 text-white shadow-xs'
                  : 'bg-slate-100 text-slate-600 hover:bg-slate-200'
              }`}
            >
              {filterKey === 'ALL' ? `All (${processes.length})` : filterKey}
            </button>
          ))}
        </div>

        {/* Search and Limit Controls */}
        <div className="flex items-center space-x-3">
          <div className="relative w-64">
            <Search className="w-3.5 h-3.5 absolute left-3 top-1/2 -translate-y-1/2 text-slate-400" />
            <input
              type="text"
              placeholder="Filter by name or PID..."
              value={searchQuery}
              onChange={(e) => { onSearchChange(e.target.value); setCurrentPage(1); }}
              className="w-full bg-slate-50 border border-slate-200 rounded-md pl-8 pr-3 py-1.5 text-xs text-slate-800 placeholder-slate-400 focus:outline-hidden focus:ring-1 focus:ring-emerald-500 focus:border-emerald-500"
            />
          </div>

          <div className="flex items-center space-x-1 text-xs text-slate-500">
            <span className="text-[11px]">Show:</span>
            <select
              value={pageSize}
              onChange={(e) => { setPageSize(Number(e.target.value)); setCurrentPage(1); }}
              className="bg-slate-50 border border-slate-200 rounded px-2 py-1 text-xs cursor-pointer focus:outline-hidden"
            >
              <option value={15}>15</option>
              <option value={25}>25</option>
              <option value={50}>50</option>
              <option value={100}>100</option>
            </select>
          </div>
        </div>
      </div>

      {/* Main Process Data Table */}
      <div className="bg-white rounded-lg border border-slate-200 shadow-xs overflow-hidden">
        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs border-collapse">
            <thead>
              <tr className="bg-slate-100/75 border-b border-slate-200 text-slate-600 font-semibold select-none">
                <th
                  onClick={() => handleSort('pid')}
                  className="py-2.5 px-3 cursor-pointer group hover:bg-slate-200/50 transition-colors w-20"
                >
                  <div className="flex items-center space-x-1 font-mono">
                    <span>PID</span>
                    {renderSortIcon('pid')}
                  </div>
                </th>
                <th
                  onClick={() => handleSort('name')}
                  className="py-2.5 px-3 cursor-pointer group hover:bg-slate-200/50 transition-colors"
                >
                  <div className="flex items-center space-x-1">
                    <span>Process Name</span>
                    {renderSortIcon('name')}
                  </div>
                </th>
                <th
                  onClick={() => handleSort('state')}
                  className="py-2.5 px-3 cursor-pointer group hover:bg-slate-200/50 transition-colors w-28"
                >
                  <div className="flex items-center space-x-1">
                    <span>State</span>
                    {renderSortIcon('state')}
                  </div>
                </th>
                <th
                  onClick={() => handleSort('cpu_pct')}
                  className="py-2.5 px-3 cursor-pointer group hover:bg-slate-200/50 transition-colors text-right w-32"
                >
                  <div className="flex items-center justify-end space-x-1">
                    <span>CPU %</span>
                    {renderSortIcon('cpu_pct')}
                  </div>
                </th>
                <th
                  onClick={() => handleSort('mem_kb')}
                  className="py-2.5 px-3 cursor-pointer group hover:bg-slate-200/50 transition-colors text-right w-32"
                >
                  <div className="flex items-center justify-end space-x-1">
                    <span>Memory (RSS)</span>
                    {renderSortIcon('mem_kb')}
                  </div>
                </th>
                <th
                  onClick={() => handleSort('mem_pct')}
                  className="py-2.5 px-3 cursor-pointer group hover:bg-slate-200/50 transition-colors text-right w-24 hidden md:table-cell"
                >
                  <div className="flex items-center justify-end space-x-1">
                    <span>% RAM</span>
                    {renderSortIcon('mem_pct')}
                  </div>
                </th>
                <th
                  onClick={() => handleSort('threads')}
                  className="py-2.5 px-3 cursor-pointer group hover:bg-slate-200/50 transition-colors text-center w-20 hidden lg:table-cell"
                >
                  <div className="flex items-center justify-center space-x-1">
                    <span>Threads</span>
                    {renderSortIcon('threads')}
                  </div>
                </th>
                <th
                  onClick={() => handleSort('ppid')}
                  className="py-2.5 px-3 cursor-pointer group hover:bg-slate-200/50 transition-colors text-center w-20 hidden lg:table-cell"
                >
                  <div className="flex items-center justify-center space-x-1">
                    <span>PPID</span>
                    {renderSortIcon('ppid')}
                  </div>
                </th>
                <th className="py-2.5 px-3 text-center w-24">Actions</th>
              </tr>
            </thead>

            <tbody className="divide-y divide-slate-100 font-sans">
              {paginatedProcesses.length > 0 ? (
                paginatedProcesses.map((proc) => {
                  const memMb = (proc.mem_kb / 1024).toFixed(1);
                  return (
                    <tr
                      key={proc.pid}
                      className="hover:bg-slate-50/80 transition-colors group cursor-pointer"
                      onClick={() => onInspectProcess(proc.pid)}
                    >
                      <td className="py-2.5 px-3 font-mono font-medium text-slate-900">
                        {proc.pid}
                      </td>

                      <td className="py-2.5 px-3 font-medium text-slate-800 truncate max-w-xs">
                        <div className="flex items-center space-x-1.5">
                          <Terminal className="w-3.5 h-3.5 text-slate-400 group-hover:text-emerald-600 shrink-0" />
                          <span className="truncate group-hover:text-emerald-700 transition-colors">
                            {proc.name}
                          </span>
                        </div>
                      </td>

                      <td className="py-2.5 px-3">
                        {getStateBadge(proc.state, proc.state_full)}
                      </td>

                      <td className="py-2.5 px-3 text-right font-mono">
                        <div className="flex items-center justify-end space-x-2">
                          <span className={`font-semibold ${proc.cpu_pct > 5 ? 'text-emerald-600' : 'text-slate-700'}`}>
                            {proc.cpu_pct.toFixed(1)}%
                          </span>
                          <div className="w-12 bg-slate-100 rounded-full h-1.5 overflow-hidden hidden sm:block">
                            <div
                              className="h-1.5 bg-emerald-500 rounded-full"
                              style={{ width: `${Math.min(100, proc.cpu_pct * 4)}%` }}
                            />
                          </div>
                        </div>
                      </td>

                      <td className="py-2.5 px-3 text-right font-mono font-medium text-slate-700">
                        {memMb} MB
                      </td>

                      <td className="py-2.5 px-3 text-right font-mono text-slate-500 hidden md:table-cell">
                        {proc.mem_pct.toFixed(2)}%
                      </td>

                      <td className="py-2.5 px-3 text-center font-mono text-slate-600 hidden lg:table-cell">
                        {proc.threads}
                      </td>

                      <td className="py-2.5 px-3 text-center font-mono text-slate-400 hidden lg:table-cell">
                        {proc.ppid}
                      </td>

                      <td className="py-2.5 px-3 text-center" onClick={(e) => e.stopPropagation()}>
                        <button
                          onClick={() => onInspectProcess(proc.pid)}
                          className="px-2 py-1 bg-slate-100 hover:bg-emerald-600 hover:text-white text-slate-700 rounded text-[11px] font-medium transition-colors flex items-center space-x-1 mx-auto cursor-pointer"
                        >
                          <span>Inspect</span>
                          <ExternalLink className="w-2.5 h-2.5" />
                        </button>
                      </td>
                    </tr>
                  );
                })
              ) : (
                <tr>
                  <td colSpan={9} className="py-12 text-center text-slate-400">
                    <Info className="w-8 h-8 text-slate-300 mx-auto mb-2" />
                    <p className="text-sm font-medium text-slate-600">No matching processes found</p>
                    <p className="text-xs text-slate-400 mt-0.5">
                      {searchQuery ? `No processes match query "${searchQuery}"` : 'Process table returned 0 items'}
                    </p>
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>

        {/* Table Footer with Pagination */}
        <div className="px-4 py-3 bg-slate-50 border-t border-slate-200 flex flex-col sm:flex-row items-center justify-between text-xs text-slate-500 gap-2">
          <div>
            Showing <span className="font-semibold text-slate-800">{sortedProcesses.length > 0 ? (currentPage - 1) * pageSize + 1 : 0}</span> to{' '}
            <span className="font-semibold text-slate-800">{Math.min(currentPage * pageSize, sortedProcesses.length)}</span> of{' '}
            <span className="font-semibold text-slate-800">{sortedProcesses.length}</span> filtered processes
            {sortedProcesses.length !== totalProcesses && ` (from ${totalProcesses} total)`}
          </div>

          <div className="flex items-center space-x-1">
            <button
              onClick={() => setCurrentPage(p => Math.max(1, p - 1))}
              disabled={currentPage <= 1}
              className="px-2 py-1 rounded border border-slate-200 bg-white hover:bg-slate-100 disabled:opacity-40 disabled:hover:bg-white text-slate-700 font-medium transition-colors cursor-pointer"
            >
              <ChevronLeft className="w-3.5 h-3.5" />
            </button>

            <span className="px-2 text-xs font-mono text-slate-600">
              Page {currentPage} of {totalPages}
            </span>

            <button
              onClick={() => setCurrentPage(p => Math.min(totalPages, p + 1))}
              disabled={currentPage >= totalPages}
              className="px-2 py-1 rounded border border-slate-200 bg-white hover:bg-slate-100 disabled:opacity-40 disabled:hover:bg-white text-slate-700 font-medium transition-colors cursor-pointer"
            >
              <ChevronRight className="w-3.5 h-3.5" />
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
