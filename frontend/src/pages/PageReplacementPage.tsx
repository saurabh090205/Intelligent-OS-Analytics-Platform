import React, { useState, useEffect, useCallback } from 'react';
import {
  Layers,
  Play,
  RotateCcw,
  Sparkles,
  BarChart3,
  CheckCircle2,
  XCircle,
  AlertTriangle,
  Sliders,
  Dices
} from 'lucide-react';
import { api } from '../services/api';
import type {
  PageSimulationResponse,
  PageSimResult,
  PageExample
} from '../types/system';

const DEFAULT_REFERENCE_STRING = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1];

export const PageReplacementPage: React.FC = () => {
  const [refInputStr, setRefInputStr] = useState<string>(DEFAULT_REFERENCE_STRING.join(', '));
  const [frameCount, setFrameCount] = useState<number>(3);
  const [examples, setExamples] = useState<PageExample[]>([]);
  const [simulation, setSimulation] = useState<PageSimulationResponse | null>(null);
  const [activeAlgoKey, setActiveAlgoKey] = useState<keyof PageSimulationResponse['algorithms']>('fifo');
  const [isLoading, setIsLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);

  // Load examples from C backend on mount
  useEffect(() => {
    api.getPageReplacementExamples()
      .then((res) => {
        if (res.examples && res.examples.length > 0) {
          setExamples(res.examples);
        }
      })
      .catch(() => {
        // Fallback silently if offline
      });
  }, []);

  const parseReferenceString = (input: string): number[] => {
    return input
      .split(/[\s,]+/)
      .map((s) => s.trim())
      .filter((s) => s.length > 0 && !isNaN(Number(s)))
      .map((s) => Math.max(0, parseInt(s, 10)));
  };

  const runSimulation = useCallback(async (refs: number[], frames: number) => {
    setIsLoading(true);
    setError(null);
    try {
      if (refs.length === 0) {
        setError('Reference string cannot be empty. Please enter integer page numbers.');
        setIsLoading(false);
        return;
      }
      if (frames <= 0 || frames > 16) {
        setError('Frame count must be between 1 and 16.');
        setIsLoading(false);
        return;
      }

      const res = await api.simulatePageReplacement({
        reference_string: refs,
        frame_count: frames,
      });

      setSimulation(res);
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Page simulation failed');
    } finally {
      setIsLoading(false);
    }
  }, []);

  // Initial simulation run
  useEffect(() => {
    const parsed = parseReferenceString(refInputStr);
    runSimulation(parsed, frameCount);
  }, [runSimulation]);

  const handleSimulateClick = () => {
    const parsed = parseReferenceString(refInputStr);
    runSimulation(parsed, frameCount);
  };

  const handleLoadExample = (ex: PageExample) => {
    setRefInputStr(ex.references.join(', '));
    setFrameCount(ex.frames);
    runSimulation(ex.references, ex.frames);
  };

  const handleGenerateRandom = () => {
    const randomRefs = Array.from({ length: 18 }, () => Math.floor(Math.random() * 8));
    setRefInputStr(randomRefs.join(', '));
    runSimulation(randomRefs, frameCount);
  };

  const currentResult: PageSimResult | undefined = simulation?.algorithms[activeAlgoKey];

  return (
    <div className="space-y-6">
      {/* Header Banner */}
      <div className="bg-white rounded-xl border border-slate-200 p-6 shadow-2xs">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div className="flex items-center space-x-3">
            <div className="w-10 h-10 rounded-lg bg-emerald-500/10 border border-emerald-500/20 flex items-center justify-center text-emerald-600">
              <Layers className="w-6 h-6" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h1 className="text-xl font-bold text-slate-900 tracking-tight">
                  Page Replacement Algorithm Simulator & Comparative Benchmark
                </h1>
                <span className="px-2 py-0.5 text-xs font-semibold rounded bg-emerald-100 text-emerald-800">
                  C POSIX Backend
                </span>
              </div>
              <p className="text-xs text-slate-500 mt-0.5">
                Exact frame-by-frame memory simulation for FIFO, LRU, and Optimal page replacement with Belady's anomaly analysis.
              </p>
            </div>
          </div>

          <div className="flex items-center space-x-2">
            <button
              onClick={handleSimulateClick}
              disabled={isLoading}
              className="px-4 py-2 bg-emerald-600 hover:bg-emerald-700 text-white rounded-lg text-xs font-semibold flex items-center space-x-2 shadow-xs transition-colors cursor-pointer disabled:opacity-50"
            >
              <Play className="w-3.5 h-3.5 fill-current" />
              <span>{isLoading ? 'Simulating...' : 'Simulate'}</span>
            </button>
            <button
              onClick={handleGenerateRandom}
              className="px-3 py-2 bg-slate-100 hover:bg-slate-200 text-slate-700 rounded-lg text-xs font-semibold flex items-center space-x-1.5 transition-colors cursor-pointer"
            >
              <Dices className="w-3.5 h-3.5" />
              <span>Random</span>
            </button>
            <button
              onClick={() => {
                setRefInputStr(DEFAULT_REFERENCE_STRING.join(', '));
                setFrameCount(3);
                runSimulation(DEFAULT_REFERENCE_STRING, 3);
              }}
              className="px-3 py-2 bg-slate-100 hover:bg-slate-200 text-slate-700 rounded-lg text-xs font-semibold flex items-center space-x-1.5 transition-colors cursor-pointer"
            >
              <RotateCcw className="w-3.5 h-3.5" />
              <span>Reset</span>
            </button>
          </div>
        </div>

        {/* Input Configuration Controls */}
        <div className="mt-5 pt-4 border-t border-slate-100 space-y-3">
          <div className="grid grid-cols-1 md:grid-cols-4 gap-4 items-center">
            {/* Reference String Input */}
            <div className="md:col-span-3 space-y-1">
              <label className="text-xs font-semibold text-slate-700 flex items-center justify-between">
                <span>Page Reference String (Comma or space separated)</span>
                <span className="text-slate-400 font-normal text-[11px]">
                  Length: {parseReferenceString(refInputStr).length} pages
                </span>
              </label>
              <input
                type="text"
                value={refInputStr}
                onChange={(e) => setRefInputStr(e.target.value)}
                placeholder="7, 0, 1, 2, 0, 3, 0, 4..."
                className="w-full px-3 py-2 border border-slate-200 rounded-lg text-xs font-mono text-slate-800 focus:ring-2 focus:ring-emerald-500 focus:border-emerald-500"
              />
            </div>

            {/* Frame Count Slider */}
            <div className="space-y-1">
              <div className="flex justify-between text-xs">
                <span className="font-semibold text-slate-700 flex items-center gap-1">
                  <Sliders className="w-3.5 h-3.5 text-slate-400" /> Frames:
                </span>
                <strong className="text-slate-900 font-mono">{frameCount} slots</strong>
              </div>
              <input
                type="range"
                min="1"
                max="8"
                step="1"
                value={frameCount}
                onChange={(e) => {
                  const f = parseInt(e.target.value, 10);
                  setFrameCount(f);
                  runSimulation(parseReferenceString(refInputStr), f);
                }}
                className="w-full accent-emerald-600 cursor-pointer"
              />
              <div className="flex justify-between text-[10px] text-slate-400">
                <span>1 Frame</span>
                <span>8 Frames</span>
              </div>
            </div>
          </div>

          {/* Preset Workloads */}
          {examples.length > 0 && (
            <div className="flex flex-wrap items-center gap-2 pt-2">
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
      </div>

      {error && (
        <div className="bg-rose-50 border border-rose-200 rounded-lg p-3 text-xs text-rose-700 flex items-center space-x-2">
          <AlertTriangle className="w-4 h-4 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      {/* Comparative Summary Banner */}
      {simulation && (
        <div className="bg-gradient-to-r from-slate-900 to-emerald-950 rounded-xl p-5 text-white shadow-md border border-emerald-800/40">
          <div className="flex items-start space-x-3">
            <div className="p-2 rounded-lg bg-emerald-500/20 border border-emerald-400/30 text-emerald-300 shrink-0">
              <CheckCircle2 className="w-5 h-5" />
            </div>
            <div className="space-y-1">
              <div className="flex items-center space-x-2">
                <span className="text-[10px] font-bold uppercase tracking-wider text-emerald-400 bg-emerald-950/80 px-2 py-0.5 rounded border border-emerald-500/30">
                  Optimal Theoretical Benchmark
                </span>
                <span className="text-sm font-bold text-white">
                  {simulation.best_algo_name} (Lowest Faults: {simulation.min_faults})
                </span>
              </div>
              <p className="text-xs text-slate-200 leading-relaxed pt-1">
                {simulation.comparison_summary}
              </p>
            </div>
          </div>
        </div>
      )}

      {/* Algorithm Comparison Cards: FIFO vs LRU vs Optimal */}
      {simulation && (
        <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
          {[
            { key: 'fifo' as const, name: 'First-In First-Out (FIFO)', res: simulation.algorithms.fifo, color: 'blue' },
            { key: 'lru' as const, name: 'Least Recently Used (LRU)', res: simulation.algorithms.lru, color: 'emerald' },
            { key: 'optimal' as const, name: 'Optimal Page Replacement (OPT)', res: simulation.algorithms.optimal, color: 'purple' },
          ].map((card) => {
            const isSelected = activeAlgoKey === card.key;
            const isBest = card.res.fault_count === simulation.min_faults;

            return (
              <div
                key={card.key}
                onClick={() => setActiveAlgoKey(card.key)}
                className={`rounded-xl border p-5 cursor-pointer transition-all shadow-2xs ${
                  isSelected
                    ? 'border-emerald-500 bg-emerald-50/20 ring-2 ring-emerald-500/30'
                    : 'border-slate-200 bg-white hover:border-slate-300 hover:shadow-xs'
                }`}
              >
                <div className="flex items-center justify-between pb-2 border-b border-slate-100">
                  <span className="text-xs font-bold text-slate-900 truncate">{card.name}</span>
                  {isBest && (
                    <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-emerald-100 text-emerald-800">
                      Best
                    </span>
                  )}
                </div>

                <div className="grid grid-cols-2 gap-4 py-4">
                  <div className="space-y-0.5">
                    <span className="text-[11px] text-slate-500">Page Faults</span>
                    <div className="text-2xl font-bold font-mono text-rose-600">
                      {card.res.fault_count}
                    </div>
                    <span className="text-[10px] text-slate-400 font-mono">
                      {(card.res.fault_ratio * 100).toFixed(1)}% fault rate
                    </span>
                  </div>

                  <div className="space-y-0.5">
                    <span className="text-[11px] text-slate-500">Page Hits</span>
                    <div className="text-2xl font-bold font-mono text-emerald-600">
                      {card.res.hit_count}
                    </div>
                    <span className="text-[10px] text-slate-400 font-mono">
                      {(card.res.hit_ratio * 100).toFixed(1)}% hit rate
                    </span>
                  </div>
                </div>

                {/* Progress bar visual */}
                <div className="space-y-1">
                  <div className="h-2 w-full bg-slate-100 rounded-full overflow-hidden flex">
                    <div
                      style={{ width: `${card.res.hit_ratio * 100}%` }}
                      className="bg-emerald-500 h-full"
                      title={`Hits: ${(card.res.hit_ratio * 100).toFixed(1)}%`}
                    />
                    <div
                      style={{ width: `${card.res.fault_ratio * 100}%` }}
                      className="bg-rose-400 h-full"
                      title={`Faults: ${(card.res.fault_ratio * 100).toFixed(1)}%`}
                    />
                  </div>
                  <div className="flex justify-between text-[10px] font-medium text-slate-500">
                    <span className="text-emerald-700">Hits ({(card.res.hit_ratio * 100).toFixed(0)}%)</span>
                    <span className="text-rose-700">Faults ({(card.res.fault_ratio * 100).toFixed(0)}%)</span>
                  </div>
                </div>
              </div>
            );
          })}
        </div>
      )}

      {/* Frame-by-Frame Step-by-Step Simulation Matrix */}
      {currentResult && (
        <div className="bg-white rounded-xl border border-slate-200 p-6 shadow-2xs space-y-4">
          <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 border-b border-slate-200 pb-3">
            <div>
              <h2 className="text-sm font-bold text-slate-900 flex items-center space-x-2">
                <BarChart3 className="w-4 h-4 text-emerald-600" />
                <span>Frame-by-Frame Execution State Matrix ({currentResult.algo_name})</span>
              </h2>
              <p className="text-[11px] text-slate-500">
                Tracking frame allocation, hit/fault outcome, and victim evictions across time steps.
              </p>
            </div>

            <div className="flex items-center space-x-1">
              {[
                { key: 'fifo' as const, label: 'FIFO' },
                { key: 'lru' as const, label: 'LRU' },
                { key: 'optimal' as const, label: 'Optimal' },
              ].map((tab) => (
                <button
                  key={tab.key}
                  onClick={() => setActiveAlgoKey(tab.key)}
                  className={`px-3 py-1 text-xs font-semibold rounded-md transition-colors cursor-pointer ${
                    activeAlgoKey === tab.key
                      ? 'bg-slate-900 text-white'
                      : 'text-slate-600 hover:text-slate-900 hover:bg-slate-100'
                  }`}
                >
                  {tab.label}
                </button>
              ))}
            </div>
          </div>

          {/* Matrix Table */}
          <div className="overflow-x-auto">
            <table className="w-full text-center text-xs border border-slate-200 rounded-lg overflow-hidden">
              <thead className="bg-slate-50 text-slate-600 uppercase text-[10px] font-bold border-b border-slate-200">
                <tr>
                  <th className="py-2.5 px-2 text-left">Step</th>
                  <th className="py-2.5 px-3 text-center">Ref Page</th>
                  {Array.from({ length: currentResult.frame_capacity }).map((_, fIdx) => (
                    <th key={fIdx} className="py-2.5 px-3">
                      Frame {fIdx}
                    </th>
                  ))}
                  <th className="py-2.5 px-3">Result</th>
                  <th className="py-2.5 px-3 text-right">Victim Evicted</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100 font-mono text-xs">
                {currentResult.steps.map((step) => {
                  return (
                    <tr
                      key={step.step}
                      className={step.is_hit ? 'bg-emerald-50/30' : 'bg-rose-50/20'}
                    >
                      <td className="py-2 px-2 text-left font-sans text-slate-500 font-medium">
                        t = {step.step}
                      </td>
                      <td className="py-2 px-3 font-bold text-slate-900 text-sm">
                        {step.page}
                      </td>

                      {/* Frame contents */}
                      {step.frames.map((framePage, fIdx) => {
                        const isReplacedFrame = step.replaced_frame_idx === fIdx && !step.is_hit;

                        return (
                          <td key={fIdx} className="py-2 px-3">
                            {framePage === -1 ? (
                              <span className="text-slate-300">-</span>
                            ) : (
                              <span
                                className={`inline-block w-7 h-7 leading-7 rounded-md font-bold text-xs ${
                                  isReplacedFrame
                                    ? 'bg-amber-100 border border-amber-300 text-amber-900'
                                    : framePage === step.page && step.is_hit
                                    ? 'bg-emerald-100 border border-emerald-300 text-emerald-900'
                                    : 'bg-slate-100 text-slate-700'
                                }`}
                              >
                                {framePage}
                              </span>
                            )}
                          </td>
                        );
                      })}

                      {/* Result Badge */}
                      <td className="py-2 px-3">
                        {step.is_hit ? (
                          <span className="inline-flex items-center px-2 py-0.5 rounded text-[10px] font-bold bg-emerald-100 text-emerald-800 font-sans">
                            <CheckCircle2 className="w-3 h-3 mr-1" /> HIT
                          </span>
                        ) : (
                          <span className="inline-flex items-center px-2 py-0.5 rounded text-[10px] font-bold bg-rose-100 text-rose-800 font-sans">
                            <XCircle className="w-3 h-3 mr-1" /> FAULT
                          </span>
                        )}
                      </td>

                      {/* Evicted Victim */}
                      <td className="py-2 px-3 text-right font-sans text-xs">
                        {step.victim_page !== -1 ? (
                          <span className="text-amber-700 font-semibold font-mono">
                            Page {step.victim_page}
                          </span>
                        ) : (
                          <span className="text-slate-400 text-[11px]">-</span>
                        )}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </div>
      )}
    </div>
  );
};
