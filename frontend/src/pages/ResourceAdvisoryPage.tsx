import React, { useState, useEffect, useCallback } from 'react';
import {
  Sliders,
  AlertTriangle,
  AlertOctagon,
  Info,
  Cpu,
  Database,
  Layers,
  Terminal,
  RefreshCw,
  ArrowUpRight,
  FileCheck2
} from 'lucide-react';
import { api } from '../services/api';
import type { ResourceAdvisoryResponse } from '../types/system';

export const ResourceAdvisoryPage: React.FC = () => {
  const [report, setReport] = useState<ResourceAdvisoryResponse | null>(null);
  const [selectedCategory, setSelectedCategory] = useState<string>('ALL');
  const [isLoading, setIsLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);
  const [lastRefreshed, setLastRefreshed] = useState<Date>(new Date());

  const fetchAdvisory = useCallback(async () => {
    setIsLoading(true);
    setError(null);
    try {
      const res = await api.getResourceAdvisory();
      setReport(res);
      setLastRefreshed(new Date());
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Failed to retrieve resource advisory recommendations');
    } finally {
      setIsLoading(false);
    }
  }, []);

  useEffect(() => {
    fetchAdvisory();
  }, [fetchAdvisory]);

  const filteredItems = report?.items
    ? report.items.filter((item) => {
        if (selectedCategory === 'ALL') return true;
        return item.category.toLowerCase().includes(selectedCategory.toLowerCase());
      })
    : [];

  const getSeverityBadge = (sev: string) => {
    switch (sev.toUpperCase()) {
      case 'CRITICAL':
        return (
          <span className="px-2.5 py-0.5 rounded-full text-xs font-bold bg-rose-100 text-rose-800 border border-rose-200 flex items-center space-x-1">
            <AlertOctagon className="w-3.5 h-3.5" />
            <span>CRITICAL</span>
          </span>
        );
      case 'WARNING':
        return (
          <span className="px-2.5 py-0.5 rounded-full text-xs font-bold bg-amber-100 text-amber-800 border border-amber-200 flex items-center space-x-1">
            <AlertTriangle className="w-3.5 h-3.5" />
            <span>WARNING</span>
          </span>
        );
      case 'INFO':
      default:
        return (
          <span className="px-2.5 py-0.5 rounded-full text-xs font-bold bg-sky-100 text-sky-800 border border-sky-200 flex items-center space-x-1">
            <Info className="w-3.5 h-3.5" />
            <span>INFORMATIONAL</span>
          </span>
        );
    }
  };

  const getPressureClass = (status: string) => {
    switch (status?.toLowerCase()) {
      case 'critical':
        return 'text-rose-600 bg-rose-50 border-rose-200';
      case 'high':
      case 'warning':
      case 'elevated':
        return 'text-amber-600 bg-amber-50 border-amber-200';
      default:
        return 'text-emerald-600 bg-emerald-50 border-emerald-200';
    }
  };

  return (
    <div className="space-y-6 pb-12">
      {/* Page Header */}
      <div className="bg-white border border-slate-200 rounded-xl p-6 shadow-xs flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center space-x-3 mb-1">
            <div className="p-2 bg-indigo-50 text-indigo-600 rounded-lg">
              <Sliders className="w-5 h-5" />
            </div>
            <h1 className="text-xl font-bold text-slate-800 tracking-tight">
              Advisory Resource Allocation Engine
            </h1>
            <span className="px-2.5 py-0.5 rounded-full text-xs font-semibold bg-indigo-100 text-indigo-700">
              Intelligent Advisory Engine
            </span>
          </div>
          <p className="text-sm text-slate-500">
            Synthesizes live host telemetry, linear regression forecasts, and algorithm benchmarks into explainable systems guidance.
          </p>
        </div>

        <div className="flex items-center space-x-3">
          <span className="text-xs text-slate-400 hidden sm:inline">
            Updated {lastRefreshed.toLocaleTimeString()}
          </span>
          <button
            onClick={fetchAdvisory}
            disabled={isLoading}
            className="flex items-center space-x-2 px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white font-medium rounded-lg shadow-xs transition-colors cursor-pointer text-sm disabled:opacity-50"
          >
            <RefreshCw className={`w-4 h-4 ${isLoading ? 'animate-spin' : ''}`} />
            <span>{isLoading ? 'Synthesizing...' : 'Re-Evaluate Advisory'}</span>
          </button>
        </div>
      </div>

      {/* Advisory Boundary Disclaimer Banner */}
      <div className="bg-gradient-to-r from-blue-500/10 via-indigo-500/5 to-transparent border-l-4 border-indigo-600 rounded-r-xl p-4 bg-white shadow-xs">
        <div className="flex items-start space-x-3">
          <FileCheck2 className="w-5 h-5 text-indigo-600 shrink-0 mt-0.5" />
          <div className="text-sm">
            <h4 className="font-semibold text-slate-800">
              Strict Advisory Boundaries &amp; Systems Safety Policy
            </h4>
            <p className="text-xs text-slate-600 mt-1 leading-relaxed">
              {report?.disclaimer ||
                'All recommendations generated by this module are non-destructive, analytical guidance intended for system administrators. The platform strictly enforces safety boundaries: it does NOT automatically terminate processes, modify kernel sysctl tunables, or alter nice priorities without operator confirmation.'}
            </p>
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

      {/* Subsystem Pressure Overview KPI Cards */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        {/* CPU Pressure */}
        <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
          <div className="flex items-center justify-between mb-2">
            <span className="text-xs font-semibold text-slate-500 uppercase tracking-wider">
              CPU Pressure
            </span>
            <Cpu className="w-4 h-4 text-slate-400" />
          </div>
          <div className="flex items-center space-x-2">
            <span className={`px-2.5 py-0.5 rounded text-xs font-bold border ${getPressureClass(report?.overview.cpu_pressure || 'Nominal')}`}>
              {report?.overview.cpu_pressure || 'Nominal'}
            </span>
          </div>
          <p className="text-[11px] text-slate-400 mt-2">
            Core utilization &amp; scheduling queue
          </p>
        </div>

        {/* Memory Pressure */}
        <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
          <div className="flex items-center justify-between mb-2">
            <span className="text-xs font-semibold text-slate-500 uppercase tracking-wider">
              Memory Pressure
            </span>
            <Database className="w-4 h-4 text-slate-400" />
          </div>
          <div className="flex items-center space-x-2">
            <span className={`px-2.5 py-0.5 rounded text-xs font-bold border ${getPressureClass(report?.overview.memory_pressure || 'Nominal')}`}>
              {report?.overview.memory_pressure || 'Nominal'}
            </span>
          </div>
          <p className="text-[11px] text-slate-400 mt-2">
            RAM headroom &amp; linear exhaustion rate
          </p>
        </div>

        {/* Process Pressure */}
        <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
          <div className="flex items-center justify-between mb-2">
            <span className="text-xs font-semibold text-slate-500 uppercase tracking-wider">
              Process Pressure
            </span>
            <Terminal className="w-4 h-4 text-slate-400" />
          </div>
          <div className="flex items-center space-x-2">
            <span className={`px-2.5 py-0.5 rounded text-xs font-bold border ${getPressureClass(report?.overview.process_pressure || 'Nominal')}`}>
              {report?.overview.process_pressure || 'Nominal'}
            </span>
          </div>
          <p className="text-[11px] text-slate-400 mt-2">
            Resident threads &amp; dominance skew
          </p>
        </div>

        {/* Paging / Swap Pressure */}
        <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
          <div className="flex items-center justify-between mb-2">
            <span className="text-xs font-semibold text-slate-500 uppercase tracking-wider">
              Paging Pressure
            </span>
            <Layers className="w-4 h-4 text-slate-400" />
          </div>
          <div className="flex items-center space-x-2">
            <span className={`px-2.5 py-0.5 rounded text-xs font-bold border ${getPressureClass(report?.overview.paging_pressure || 'Nominal')}`}>
              {report?.overview.paging_pressure || 'Nominal'}
            </span>
          </div>
          <p className="text-[11px] text-slate-400 mt-2">
            Swap migration &amp; cache fault ratio
          </p>
        </div>
      </div>

      {/* Filter Tabs */}
      <div className="flex items-center space-x-2 border-b border-slate-200 pb-3">
        {['ALL', 'CPU', 'Memory', 'Process', 'Paging'].map((cat) => (
          <button
            key={cat}
            onClick={() => setSelectedCategory(cat)}
            className={`px-3 py-1.5 rounded-lg text-xs font-semibold transition-colors cursor-pointer ${
              selectedCategory === cat
                ? 'bg-indigo-600 text-white shadow-xs'
                : 'bg-slate-100 hover:bg-slate-200 text-slate-600'
            }`}
          >
            {cat === 'ALL' ? 'All Subsystems' : `${cat} Guidance`}
          </button>
        ))}
      </div>

      {/* Actionable Recommendations List */}
      <div className="space-y-4">
        {filteredItems.length > 0 ? (
          filteredItems.map((item) => (
            <div
              key={item.id}
              className="bg-white border border-slate-200 rounded-xl p-5 shadow-xs hover:border-slate-300 transition-all space-y-3"
            >
              {/* Header row */}
              <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2">
                <div className="flex items-center space-x-3">
                  {getSeverityBadge(item.severity)}
                  <h3 className="text-sm font-bold text-slate-800">{item.title}</h3>
                </div>

                <div className="flex items-center space-x-3 text-xs">
                  {/* Telemetry vs Estimate indicator */}
                  {item.is_pure_telemetry ? (
                    <span className="px-2 py-0.5 bg-emerald-50 text-emerald-700 rounded text-[10px] font-semibold border border-emerald-200">
                      Measured Telemetry
                    </span>
                  ) : (
                    <span className="px-2 py-0.5 bg-purple-50 text-purple-700 rounded text-[10px] font-semibold border border-purple-200">
                      Statistical Model / Simulation
                    </span>
                  )}
                  <span className="text-slate-400 font-mono">
                    Confidence: <strong>{item.confidence_pct}%</strong>
                  </span>
                </div>
              </div>

              {/* Rationale and Supporting Measurements */}
              <div className="text-xs text-slate-600 space-y-1">
                <div>
                  <strong className="text-slate-700 font-semibold">Supporting Rationale: </strong>
                  <span>{item.rationale}</span>
                </div>

                {item.metric_name && (
                  <div className="flex items-center space-x-4 pt-1 font-mono text-[11px]">
                    <span className="text-slate-500">
                      Metric: <strong className="text-slate-700">{item.metric_name}</strong>
                    </span>
                    <span className="text-slate-500">
                      Measured / Analyzed:{' '}
                      <strong className="text-indigo-600">{item.measured_value}</strong>
                    </span>
                    <span className="text-slate-500">
                      Operating Threshold:{' '}
                      <strong className="text-slate-700">{item.threshold_value}</strong>
                    </span>
                  </div>
                )}
              </div>

              {/* Recommended Action box */}
              <div className="bg-slate-50 border border-slate-200 rounded-lg p-3 text-xs flex items-start space-x-2.5">
                <ArrowUpRight className="w-4 h-4 text-indigo-600 shrink-0 mt-0.5" />
                <div>
                  <span className="font-bold text-slate-800 uppercase tracking-wider text-[10px] block mb-0.5">
                    Recommended Systems Action (Advisory Only):
                  </span>
                  <span className="text-slate-700 leading-relaxed font-medium">
                    {item.recommended_action}
                  </span>
                </div>
              </div>
            </div>
          ))
        ) : (
          <div className="p-12 text-center bg-white border border-slate-200 rounded-xl text-xs text-slate-400">
            No active recommendations for the selected category.
          </div>
        )}
      </div>
    </div>
  );
};
