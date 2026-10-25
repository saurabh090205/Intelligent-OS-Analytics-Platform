import React, { useState, useEffect, useCallback } from 'react';
import {
  Database,
  TrendingUp,
  TrendingDown,
  Minus,
  AlertTriangle,
  Clock,
  Sliders,
  RefreshCw,
  Info,
  ShieldAlert
} from 'lucide-react';
import {
  ResponsiveContainer,
  ComposedChart,
  Line,
  XAxis,
  YAxis,
  Tooltip,
  CartesianGrid,
  ReferenceLine
} from 'recharts';
import { api } from '../services/api';
import type {
  MemoryForecastResponse,
  MemoryHistorySample
} from '../types/system';

export const MemoryAnalyticsPage: React.FC = () => {
  const [horizon, setHorizon] = useState<number>(60);
  const [threshold, setThreshold] = useState<number>(85.0);
  const [windowSize, setWindowSize] = useState<number>(10);
  const [forecast, setForecast] = useState<MemoryForecastResponse | null>(null);
  const [historySamples, setHistorySamples] = useState<MemoryHistorySample[]>([]);
  const [isLoading, setIsLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);

  const fetchForecast = useCallback(async () => {
    setIsLoading(true);
    setError(null);
    try {
      const [fc, hist] = await Promise.all([
        api.getMemoryForecast(horizon, windowSize, threshold),
        api.getMemoryHistory(),
      ]);

      setForecast(fc);
      if (hist && hist.samples) {
        setHistorySamples(hist.samples);
      }
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Failed to fetch memory forecast');
    } finally {
      setIsLoading(false);
    }
  }, [horizon, windowSize, threshold]);

  useEffect(() => {
    fetchForecast();
    const interval = setInterval(fetchForecast, 3000);
    return () => clearInterval(interval);
  }, [fetchForecast]);

  // Combine actual history and forecast points for Recharts visualization
  const chartData: Array<{
    label: string;
    actual?: number;
    predicted?: number;
    threshold?: number;
    timestamp: number;
    isForecast?: boolean;
  }> = [];

  // 1. Add historical actual telemetry points
  historySamples.slice(-20).forEach((sample) => {
    const d = new Date(sample.timestamp * 1000);
    chartData.push({
      label: d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
      actual: parseFloat(sample.utilization_pct.toFixed(2)),
      threshold,
      timestamp: sample.timestamp,
      isForecast: false,
    });
  });

  // 2. Add forecast projected points
  if (forecast && forecast.forecast_points && forecast.forecast_points.length > 0) {
    forecast.forecast_points.forEach((pt) => {
      chartData.push({
        label: `+${pt.offset_seconds}s`,
        predicted: parseFloat(pt.predicted_util_pct.toFixed(2)),
        threshold,
        timestamp: pt.timestamp,
        isForecast: true,
      });
    });
  }

  const formatGb = (kb: number) => {
    return (kb / (1024 * 1024)).toFixed(2) + ' GB';
  };

  const getTrendIcon = (trendId?: number) => {
    if (!trendId || trendId === 0) return <Minus className="w-4 h-4 text-slate-500" />;
    if (trendId > 0) return <TrendingUp className="w-4 h-4 text-rose-500" />;
    return <TrendingDown className="w-4 h-4 text-emerald-500" />;
  };

  const getTrendBadge = (trendId?: number, label?: string) => {
    if (!trendId || trendId === 0) {
      return (
        <span className="px-2.5 py-1 bg-slate-100 text-slate-700 rounded-full text-xs font-semibold flex items-center space-x-1">
          <Minus className="w-3.5 h-3.5" />
          <span>{label || 'Stable'}</span>
        </span>
      );
    }
    if (trendId > 0) {
      return (
        <span className="px-2.5 py-1 bg-rose-100 text-rose-700 rounded-full text-xs font-semibold flex items-center space-x-1">
          <TrendingUp className="w-3.5 h-3.5" />
          <span>{label || 'Increasing'}</span>
        </span>
      );
    }
    return (
      <span className="px-2.5 py-1 bg-emerald-100 text-emerald-700 rounded-full text-xs font-semibold flex items-center space-x-1">
        <TrendingDown className="w-3.5 h-3.5" />
        <span>{label || 'Decreasing'}</span>
      </span>
    );
  };

  return (
    <div className="space-y-6">
      {/* Header Banner */}
      <div className="bg-white rounded-xl border border-slate-200 p-6 shadow-2xs">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div className="flex items-center space-x-3">
            <div className="w-10 h-10 rounded-lg bg-emerald-500/10 border border-emerald-500/20 flex items-center justify-center text-emerald-600">
              <Database className="w-6 h-6" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h1 className="text-xl font-bold text-slate-900 tracking-tight">
                  Memory Utilization Prediction & Trend Forecasting
                </h1>
                <span className="px-2 py-0.5 text-xs font-semibold rounded bg-emerald-100 text-emerald-800">
                  C Statistical Engine
                </span>
              </div>
              <p className="text-xs text-slate-500 mt-0.5">
                Moving average and simple linear regression computed on actual sampled OS memory telemetry.
              </p>
            </div>
          </div>

          <div className="flex items-center space-x-2">
            <button
              onClick={fetchForecast}
              disabled={isLoading}
              className="px-3.5 py-2 bg-slate-100 hover:bg-slate-200 text-slate-700 rounded-lg text-xs font-semibold flex items-center space-x-1.5 transition-colors cursor-pointer"
            >
              <RefreshCw className={`w-3.5 h-3.5 ${isLoading ? 'animate-spin' : ''}`} />
              <span>Refresh Telemetry</span>
            </button>
          </div>
        </div>

        {/* Horizon, Smoothing & Threshold Controls */}
        <div className="mt-5 pt-4 border-t border-slate-100 grid grid-cols-1 sm:grid-cols-3 gap-4">
          {/* Horizon Slider */}
          <div className="space-y-1.5">
            <div className="flex justify-between text-xs">
              <span className="font-medium text-slate-700 flex items-center gap-1">
                <Clock className="w-3.5 h-3.5 text-slate-400" /> Forecast Horizon:
              </span>
              <strong className="text-slate-900 font-mono">{horizon}s</strong>
            </div>
            <input
              type="range"
              min="15"
              max="300"
              step="15"
              value={horizon}
              onChange={(e) => setHorizon(parseInt(e.target.value))}
              className="w-full accent-emerald-600 cursor-pointer"
            />
            <div className="flex justify-between text-[10px] text-slate-400">
              <span>15s (Short-term)</span>
              <span>300s (5 min)</span>
            </div>
          </div>

          {/* Window Size Slider */}
          <div className="space-y-1.5">
            <div className="flex justify-between text-xs">
              <span className="font-medium text-slate-700 flex items-center gap-1">
                <Sliders className="w-3.5 h-3.5 text-slate-400" /> SMA Window:
              </span>
              <strong className="text-slate-900 font-mono">{windowSize} samples</strong>
            </div>
            <input
              type="range"
              min="5"
              max="30"
              step="5"
              value={windowSize}
              onChange={(e) => setWindowSize(parseInt(e.target.value))}
              className="w-full accent-emerald-600 cursor-pointer"
            />
            <div className="flex justify-between text-[10px] text-slate-400">
              <span>5 (Responsive)</span>
              <span>30 (Smoothed)</span>
            </div>
          </div>

          {/* Threshold Slider */}
          <div className="space-y-1.5">
            <div className="flex justify-between text-xs">
              <span className="font-medium text-slate-700 flex items-center gap-1">
                <AlertTriangle className="w-3.5 h-3.5 text-amber-500" /> Safety Alert Threshold:
              </span>
              <strong className="text-slate-900 font-mono">{threshold}%</strong>
            </div>
            <input
              type="range"
              min="60"
              max="95"
              step="5"
              value={threshold}
              onChange={(e) => setThreshold(parseFloat(e.target.value))}
              className="w-full accent-rose-600 cursor-pointer"
            />
            <div className="flex justify-between text-[10px] text-slate-400">
              <span>60% (Strict)</span>
              <span>95% (Relaxed)</span>
            </div>
          </div>
        </div>
      </div>

      {error && (
        <div className="bg-rose-50 border border-rose-200 rounded-lg p-3 text-xs text-rose-700 flex items-center space-x-2">
          <AlertTriangle className="w-4 h-4 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      {/* Safety Alert Notification Banner */}
      {forecast && forecast.threshold_exceeded && (
        <div className="bg-rose-50 border border-rose-300 rounded-xl p-4 text-rose-800 shadow-xs flex items-start space-x-3">
          <div className="p-2 rounded-lg bg-rose-100 text-rose-600 shrink-0">
            <ShieldAlert className="w-5 h-5" />
          </div>
          <div className="space-y-1">
            <div className="flex items-center space-x-2">
              <span className="text-xs font-bold uppercase tracking-wider text-rose-700">
                Memory Threshold Alert
              </span>
              <span className="text-xs font-semibold text-rose-900">
                (Projected {forecast.predicted_reg_util_pct.toFixed(1)}% &gt; {threshold}%)
              </span>
            </div>
            <p className="text-xs text-rose-700 leading-relaxed">
              {forecast.warning_message}
            </p>
          </div>
        </div>
      )}

      {/* Key Metric KPI Cards */}
      {forecast && (
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
          {/* Card 1: Current vs Predicted Utilization */}
          <div className="bg-white rounded-xl border border-slate-200 p-4 shadow-2xs space-y-2">
            <div className="flex items-center justify-between text-xs text-slate-500">
              <span>Actual Memory Usage</span>
              <span className="font-semibold text-slate-700 font-mono">Real OS</span>
            </div>
            <div className="flex items-baseline space-x-2">
              <span className="text-2xl font-bold text-slate-900 font-mono">
                {forecast.current_utilization_pct.toFixed(1)}%
              </span>
              <span className="text-xs text-slate-400">sampled</span>
            </div>
            <div className="text-[11px] text-slate-500 pt-1 border-t border-slate-100 flex justify-between">
              <span>Available RAM:</span>
              <span className="font-semibold font-mono text-slate-700">
                {formatGb(forecast.current_available_kb)}
              </span>
            </div>
          </div>

          {/* Card 2: Linear Regression Projection */}
          <div className="bg-white rounded-xl border border-slate-200 p-4 shadow-2xs space-y-2">
            <div className="flex items-center justify-between text-xs text-slate-500">
              <span>Projected ({forecast.forecast_horizon_seconds}s)</span>
              <span className="font-semibold text-purple-700 font-mono">Linear Model</span>
            </div>
            <div className="flex items-baseline space-x-2">
              <span className={`text-2xl font-bold font-mono ${
                forecast.predicted_reg_util_pct >= threshold ? 'text-rose-600' : 'text-purple-700'
              }`}>
                {forecast.predicted_reg_util_pct.toFixed(1)}%
              </span>
              <span className="text-xs text-slate-400">predicted</span>
            </div>
            <div className="text-[11px] text-slate-500 pt-1 border-t border-slate-100 flex justify-between">
              <span>SMA ({windowSize}) Projection:</span>
              <span className="font-semibold font-mono text-teal-700">
                {forecast.predicted_sma_util_pct.toFixed(1)}%
              </span>
            </div>
          </div>

          {/* Card 3: Memory Trend & Velocity */}
          <div className="bg-white rounded-xl border border-slate-200 p-4 shadow-2xs space-y-2">
            <div className="flex items-center justify-between text-xs text-slate-500">
              <span>Memory Trajectory</span>
              {getTrendIcon(forecast.trend_id)}
            </div>
            <div className="pt-1">
              {getTrendBadge(forecast.trend_id, forecast.trend_label)}
            </div>
            <div className="text-[11px] text-slate-500 pt-1 border-t border-slate-100 flex justify-between">
              <span>Slope (Velocity):</span>
              <span className="font-semibold font-mono text-slate-700">
                {forecast.slope_pct_per_min > 0 ? '+' : ''}{forecast.slope_pct_per_min.toFixed(2)}% / min
              </span>
            </div>
          </div>

          {/* Card 4: Model Confidence & Available RAM */}
          <div className="bg-white rounded-xl border border-slate-200 p-4 shadow-2xs space-y-2">
            <div className="flex items-center justify-between text-xs text-slate-500">
              <span>Model Fit & Available</span>
              <span className="font-mono text-slate-600 font-semibold">R² = {forecast.r_squared.toFixed(2)}</span>
            </div>
            <div className="flex items-baseline space-x-2">
              <span className="text-2xl font-bold text-slate-900 font-mono">
                {formatGb(forecast.predicted_available_kb)}
              </span>
              <span className="text-xs text-slate-400">est. free</span>
            </div>
            <div className="text-[11px] text-slate-500 pt-1 border-t border-slate-100 flex justify-between">
              <span>Total RAM:</span>
              <span className="font-semibold font-mono text-slate-700">
                {formatGb(forecast.total_ram_kb)}
              </span>
            </div>
          </div>
        </div>
      )}

      {/* Main Combined Telemetry & Forecast Recharts Chart */}
      <div className="bg-white rounded-xl border border-slate-200 p-5 shadow-2xs space-y-4">
        <div className="flex items-center justify-between">
          <div className="space-y-0.5">
            <h2 className="text-sm font-bold text-slate-900 flex items-center space-x-2">
              <TrendingUp className="w-4 h-4 text-emerald-600" />
              <span>Historical Telemetry & Extrapolated Forecast Curve</span>
            </h2>
            <p className="text-[11px] text-slate-500">
              Solid line represents recorded telemetry; dashed line indicates statistical linear forecast.
            </p>
          </div>
          <div className="flex items-center space-x-3 text-xs">
            <div className="flex items-center space-x-1.5">
              <span className="w-3 h-0.5 bg-emerald-500 inline-block" />
              <span className="text-slate-600">Actual Memory %</span>
            </div>
            <div className="flex items-center space-x-1.5">
              <span className="w-3 h-0.5 bg-purple-500 border-dashed inline-block" />
              <span className="text-slate-600">Linear Forecast</span>
            </div>
            <div className="flex items-center space-x-1.5">
              <span className="w-3 h-0.5 bg-rose-500 inline-block" />
              <span className="text-slate-600">Threshold ({threshold}%)</span>
            </div>
          </div>
        </div>

        <div className="h-72 w-full pt-2">
          {chartData.length > 0 ? (
            <ResponsiveContainer width="100%" height="100%">
              <ComposedChart data={chartData} margin={{ top: 10, right: 20, left: -10, bottom: 0 }}>
                <CartesianGrid strokeDasharray="3 3" stroke="#f1f5f9" />
                <XAxis dataKey="label" tick={{ fontSize: 10, fill: '#64748b' }} />
                <YAxis domain={[0, 100]} tick={{ fontSize: 10, fill: '#64748b' }} unit="%" />
                <Tooltip
                  formatter={(value: any, name: any) => [
                    `${typeof value === 'number' ? value.toFixed(2) : value}%`,
                    name === 'actual' ? 'Actual Telemetry' : name === 'predicted' ? 'Linear Forecast' : 'Alert Threshold',
                  ]}
                  labelStyle={{ fontWeight: 'bold', color: '#1e293b' }}
                  contentStyle={{ backgroundColor: '#ffffff', borderColor: '#e2e8f0', borderRadius: '8px', fontSize: '11px' }}
                />
                <ReferenceLine
                  y={threshold}
                  stroke="#ef4444"
                  strokeDasharray="4 4"
                  label={{ value: `Threshold ${threshold}%`, position: 'right', fill: '#ef4444', fontSize: 10 }}
                />
                <Line
                  type="monotone"
                  dataKey="actual"
                  stroke="#10b981"
                  strokeWidth={2.5}
                  dot={{ r: 3, fill: '#10b981' }}
                  name="actual"
                  isAnimationActive={false}
                />
                <Line
                  type="linear"
                  dataKey="predicted"
                  stroke="#8b5cf6"
                  strokeWidth={2.5}
                  strokeDasharray="5 5"
                  dot={{ r: 4, fill: '#8b5cf6' }}
                  name="predicted"
                  isAnimationActive={false}
                />
              </ComposedChart>
            </ResponsiveContainer>
          ) : (
            <div className="h-full flex items-center justify-center text-xs text-slate-400">
              Gathering memory telemetry samples...
            </div>
          )}
        </div>
      </div>

      {/* Academic / Statistical Limitation Banner */}
      {forecast && (
        <div className="bg-slate-50 border border-slate-200 rounded-xl p-4 text-slate-600 text-xs space-y-1.5">
          <div className="flex items-center space-x-2 text-slate-800 font-bold">
            <Info className="w-4 h-4 text-emerald-600" />
            <span>Statistical Forecasting Engine Methodology & Limitations</span>
          </div>
          <p className="text-slate-600 leading-relaxed text-[11px]">
            {forecast.limitation_explanation}
          </p>
          <p className="text-slate-500 text-[11px]">
            Total samples in memory buffer: <strong className="text-slate-700">{forecast.sample_count}</strong>.
            Sampling interval: 1s. Memory telemetry is gathered directly from host OS <code className="font-mono text-emerald-700">/proc/meminfo</code> (Linux) and <code className="font-mono text-emerald-700">GlobalMemoryStatusEx</code> (Windows).
          </p>
        </div>
      )}
    </div>
  );
};
