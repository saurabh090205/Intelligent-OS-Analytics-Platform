import React, { useState, useEffect, useCallback } from 'react';
import {
  ShieldAlert,
  Play,
  Sparkles,
  CheckCircle2,
  AlertTriangle,
  XCircle,
  Plus,
  Trash2,
  ArrowRight,
  HelpCircle,
  Cpu,
  Activity
} from 'lucide-react';
import { api } from '../services/api';
import type {
  DeadlockState,
  BankerResult,
  DeadlockRequestResult,
  DeadlockDetectionResult,
  DeadlockExample
} from '../types/system';

const INITIAL_PROCESS_COUNT = 5;
const INITIAL_RESOURCE_COUNT = 3;

export const DeadlockPage: React.FC = () => {
  const [numProcesses, setNumProcesses] = useState<number>(INITIAL_PROCESS_COUNT);
  const [numResources, setNumResources] = useState<number>(INITIAL_RESOURCE_COUNT);
  const [processNames, setProcessNames] = useState<string[]>(['P0', 'P1', 'P2', 'P3', 'P4']);
  const [resourceNames, setResourceNames] = useState<string[]>(['R0', 'R1', 'R2']);

  const [available, setAvailable] = useState<number[]>([3, 3, 2]);
  const [allocation, setAllocation] = useState<number[][]>([
    [0, 1, 0],
    [2, 0, 0],
    [3, 0, 2],
    [2, 1, 1],
    [0, 0, 2]
  ]);
  const [maxClaim, setMaxClaim] = useState<number[][]>([
    [7, 5, 3],
    [3, 2, 2],
    [9, 0, 2],
    [2, 2, 2],
    [4, 3, 3]
  ]);

  // Request Evaluation state
  const [requestProcessIdx, setRequestProcessIdx] = useState<number>(1);
  const [requestVector, setRequestVector] = useState<number[]>([1, 0, 2]);
  const [requestResult, setRequestResult] = useState<DeadlockRequestResult | null>(null);

  // Analysis results
  const [bankerResult, setBankerResult] = useState<BankerResult | null>(null);
  const [detectResult, setDetectResult] = useState<DeadlockDetectionResult | null>(null);
  const [examples, setExamples] = useState<DeadlockExample[]>([]);
  const [selectedExampleId, setSelectedExampleId] = useState<string>('silberschatz_safe');
  const [isLoading, setIsLoading] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);

  // Load examples from C backend on mount
  useEffect(() => {
    api.getDeadlockExamples()
      .then((res) => {
        if (res.examples && res.examples.length > 0) {
          setExamples(res.examples);
        }
      })
      .catch(() => {});
  }, []);

  // Run Banker and Detection analysis
  const runAnalysis = useCallback(async (
    avail: number[],
    alloc: number[][],
    max: number[][],
    pCount: number,
    rCount: number
  ) => {
    setIsLoading(true);
    setError(null);
    try {
      // Client-side pre-validation
      for (let i = 0; i < pCount; i++) {
        for (let j = 0; j < rCount; j++) {
          if (alloc[i][j] < 0 || max[i][j] < 0) {
            setError(`Matrix values cannot be negative (Process ${i}, Resource ${j}).`);
            setIsLoading(false);
            return;
          }
          if (alloc[i][j] > max[i][j]) {
            setError(`Allocation exceeds Max Claim for Process ${i}, Resource ${j} (${alloc[i][j]} > ${max[i][j]}).`);
            setIsLoading(false);
            return;
          }
        }
      }
      for (let j = 0; j < rCount; j++) {
        if (avail[j] < 0) {
          setError(`Available vector cannot contain negative resources (Resource ${j}).`);
          setIsLoading(false);
          return;
        }
      }

      const payload: DeadlockState = {
        num_processes: pCount,
        num_resources: rCount,
        available: avail.slice(0, rCount),
        allocation: alloc.slice(0, pCount).map(row => row.slice(0, rCount)),
        max_claim: max.slice(0, pCount).map(row => row.slice(0, rCount)),
      };

      const [bankerRes, detectRes] = await Promise.all([
        api.analyzeBanker(payload),
        api.detectDeadlock(payload),
      ]);

      setBankerResult(bankerRes);
      setDetectResult(detectRes);
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Deadlock analysis failed');
    } finally {
      setIsLoading(false);
    }
  }, []);

  // Initial analysis run
  useEffect(() => {
    runAnalysis(available, allocation, maxClaim, numProcesses, numResources);
  }, []);

  // Load preset scenario
  const handleLoadExample = (example: DeadlockExample) => {
    setSelectedExampleId(example.id);
    setNumProcesses(example.num_processes);
    setNumResources(example.num_resources);
    setProcessNames(example.processes || Array.from({ length: example.num_processes }, (_, i) => `P${i}`));
    setResourceNames(example.resources || Array.from({ length: example.num_resources }, (_, j) => `R${j}`));
    setAvailable([...example.available]);
    setAllocation(example.allocation.map(row => [...row]));
    setMaxClaim(example.max_claim.map(row => [...row]));
    setRequestProcessIdx(0);
    setRequestVector(new Array(example.num_resources).fill(0));
    setRequestResult(null);

    runAnalysis(
      example.available,
      example.allocation,
      example.max_claim,
      example.num_processes,
      example.num_resources
    );
  };

  // Add process
  const handleAddProcess = () => {
    if (numProcesses >= 10) return;
    const newIdx = numProcesses;
    setNumProcesses(newIdx + 1);
    setProcessNames([...processNames, `P${newIdx}`]);
    setAllocation([...allocation, new Array(numResources).fill(0)]);
    setMaxClaim([...maxClaim, new Array(numResources).fill(0)]);
  };

  // Remove process
  const handleRemoveProcess = (idx: number) => {
    if (numProcesses <= 2) return;
    setNumProcesses(numProcesses - 1);
    setProcessNames(processNames.filter((_, i) => i !== idx));
    setAllocation(allocation.filter((_, i) => i !== idx));
    setMaxClaim(maxClaim.filter((_, i) => i !== idx));
    if (requestProcessIdx >= numProcesses - 1) {
      setRequestProcessIdx(Math.max(0, numProcesses - 2));
    }
  };

  // Add resource type
  const handleAddResource = () => {
    if (numResources >= 6) return;
    const newIdx = numResources;
    setNumResources(newIdx + 1);
    setResourceNames([...resourceNames, `R${newIdx}`]);
    setAvailable([...available, 1]);
    setAllocation(allocation.map(row => [...row, 0]));
    setMaxClaim(maxClaim.map(row => [...row, 1]));
    setRequestVector([...requestVector, 0]);
  };

  // Remove resource type
  const handleRemoveResource = (idx: number) => {
    if (numResources <= 1) return;
    setNumResources(numResources - 1);
    setResourceNames(resourceNames.filter((_, j) => j !== idx));
    setAvailable(available.filter((_, j) => j !== idx));
    setAllocation(allocation.map(row => row.filter((_, j) => j !== idx)));
    setMaxClaim(maxClaim.map(row => row.filter((_, j) => j !== idx)));
    setRequestVector(requestVector.filter((_, j) => j !== idx));
  };

  // Update matrix cells
  const handleAllocationChange = (pIdx: number, rIdx: number, val: number) => {
    const updated = allocation.map((row, i) =>
      i === pIdx ? row.map((c, j) => (j === rIdx ? Math.max(0, val) : c)) : row
    );
    setAllocation(updated);
  };

  const handleMaxClaimChange = (pIdx: number, rIdx: number, val: number) => {
    const updated = maxClaim.map((row, i) =>
      i === pIdx ? row.map((c, j) => (j === rIdx ? Math.max(0, val) : c)) : row
    );
    setMaxClaim(updated);
  };

  const handleAvailableChange = (rIdx: number, val: number) => {
    const updated = available.map((c, j) => (j === rIdx ? Math.max(0, val) : c));
    setAvailable(updated);
  };

  // Test Resource Request
  const handleTestRequest = async () => {
    try {
      const payload = {
        process_idx: requestProcessIdx,
        request: requestVector.slice(0, numResources),
        num_processes: numProcesses,
        num_resources: numResources,
        available: available.slice(0, numResources),
        allocation: allocation.slice(0, numProcesses).map(row => row.slice(0, numResources)),
        max_claim: maxClaim.slice(0, numProcesses).map(row => row.slice(0, numResources)),
      };
      const res = await api.checkDeadlockRequest(payload);
      setRequestResult(res);
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Request evaluation failed');
    }
  };

  // Compute local Need matrix for visualization
  const needMatrix = allocation.map((row, i) =>
    row.map((allocVal, j) => Math.max(0, (maxClaim[i]?.[j] ?? 0) - allocVal))
  );

  return (
    <div className="space-y-6 pb-12">
      {/* Page Header */}
      <div className="bg-white border border-slate-200 rounded-xl p-6 shadow-xs flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="flex items-center space-x-3 mb-1">
            <div className="p-2 bg-indigo-50 text-indigo-600 rounded-lg">
              <ShieldAlert className="w-5 h-5" />
            </div>
            <h1 className="text-xl font-bold text-slate-800 tracking-tight">
              Deadlock Detection & Avoidance
            </h1>
            <span className="px-2.5 py-0.5 rounded-full text-xs font-semibold bg-indigo-100 text-indigo-700">
              Module 5 (Phase 3)
            </span>
          </div>
          <p className="text-sm text-slate-500">
            Resource-Allocation Graph modeling, Dijkstra&apos;s Banker&apos;s Safety Algorithm, and immediate circular-wait cycle detection.
          </p>
        </div>

        <div className="flex items-center space-x-3">
          <button
            onClick={() => runAnalysis(available, allocation, maxClaim, numProcesses, numResources)}
            disabled={isLoading}
            className="flex items-center space-x-2 px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white font-medium rounded-lg shadow-xs transition-colors cursor-pointer text-sm disabled:opacity-50"
          >
            <Play className="w-4 h-4" />
            <span>{isLoading ? 'Analyzing...' : 'Run Safety Analysis'}</span>
          </button>
        </div>
      </div>

      {/* Educational Banner: Unsafe State vs Confirmed Deadlock */}
      <div className="bg-gradient-to-r from-amber-500/10 via-amber-500/5 to-transparent border-l-4 border-amber-500 rounded-r-xl p-4 bg-white shadow-xs">
        <div className="flex items-start space-x-3">
          <HelpCircle className="w-5 h-5 text-amber-600 shrink-0 mt-0.5" />
          <div className="text-sm">
            <h4 className="font-semibold text-slate-800">
              Fundamental OS Principle: Unsafe State vs. Confirmed Deadlock
            </h4>
            <div className="text-slate-600 mt-1 grid grid-cols-1 md:grid-cols-2 gap-3 text-xs leading-relaxed">
              <div className="bg-white/80 p-2.5 rounded-lg border border-amber-200/60">
                <strong className="text-amber-800 block mb-0.5 font-semibold">⚠️ Unsafe State (Avoidance Warning):</strong>
                An unsafe state does <em>not</em> mean the system is currently deadlocked. It means available resources cannot <em>guarantee</em> completion if all processes simultaneously demand their declared maximum claims. If processes only request what they currently need and release them, execution may proceed safely without deadlocking.
              </div>
              <div className="bg-white/80 p-2.5 rounded-lg border border-rose-200/60">
                <strong className="text-rose-800 block mb-0.5 font-semibold">🛑 Confirmed Deadlock (Circular Wait):</strong>
                A confirmed deadlock means an active, unresolvable circular wait condition exists right now. Process A holds resource X and waits for Y, while Process B holds resource Y and waits for X. Neither process can proceed and the system cannot recover without preemption or termination.
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Preset Scenarios Selector */}
      <div className="bg-white border border-slate-200 rounded-xl p-4 shadow-xs">
        <div className="flex items-center space-x-2 mb-3">
          <Sparkles className="w-4 h-4 text-indigo-600" />
          <span className="text-xs font-bold uppercase tracking-wider text-slate-500">
            Standard Operating System Scenarios
          </span>
        </div>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
          {examples.map((ex) => {
            const isSelected = selectedExampleId === ex.id;
            return (
              <button
                key={ex.id}
                onClick={() => handleLoadExample(ex)}
                className={`p-3 text-left rounded-lg border transition-all cursor-pointer ${
                  isSelected
                    ? 'border-indigo-600 bg-indigo-50/60 ring-2 ring-indigo-500/20'
                    : 'border-slate-200 hover:border-slate-300 bg-slate-50/50'
                }`}
              >
                <div className="flex items-center justify-between mb-1">
                  <span className="text-xs font-bold text-slate-800">{ex.title}</span>
                  {ex.id === 'silberschatz_safe' && (
                    <span className="px-1.5 py-0.5 text-[10px] bg-emerald-100 text-emerald-700 font-semibold rounded">
                      Safe State
                    </span>
                  )}
                  {ex.id === 'unsafe_state' && (
                    <span className="px-1.5 py-0.5 text-[10px] bg-amber-100 text-amber-700 font-semibold rounded">
                      Unsafe State
                    </span>
                  )}
                  {ex.id === 'deadlock_cycle' && (
                    <span className="px-1.5 py-0.5 text-[10px] bg-rose-100 text-rose-700 font-semibold rounded">
                      Deadlock Cycle
                    </span>
                  )}
                </div>
                <p className="text-[11px] text-slate-500 line-clamp-2 leading-relaxed">
                  {ex.description}
                </p>
              </button>
            );
          })}
        </div>
      </div>

      {/* Error Alert */}
      {error && (
        <div className="bg-rose-50 border border-rose-200 rounded-xl p-4 text-rose-700 text-sm flex items-center space-x-2">
          <AlertTriangle className="w-5 h-5 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      {/* Analysis Results Summary Bar */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {/* Banker's Safety Card */}
        <div className={`border rounded-xl p-5 shadow-xs transition-colors ${
          bankerResult?.is_safe
            ? 'bg-emerald-50/40 border-emerald-200'
            : 'bg-amber-50/40 border-amber-200'
        }`}>
          <div className="flex items-center justify-between mb-3">
            <div className="flex items-center space-x-2">
              {bankerResult?.is_safe ? (
                <CheckCircle2 className="w-5 h-5 text-emerald-600" />
              ) : (
                <AlertTriangle className="w-5 h-5 text-amber-600" />
              )}
              <h3 className="text-sm font-bold text-slate-800 uppercase tracking-wide">
                Banker&apos;s Safety Algorithm
              </h3>
            </div>
            <span className={`px-2.5 py-1 rounded-full text-xs font-bold ${
              bankerResult?.is_safe
                ? 'bg-emerald-100 text-emerald-800'
                : 'bg-amber-100 text-amber-800'
            }`}>
              {bankerResult?.is_safe ? 'SAFE STATE' : 'UNSAFE STATE'}
            </span>
          </div>

          <p className="text-xs text-slate-600 mb-3 leading-relaxed">
            {bankerResult?.explanation}
          </p>

          {bankerResult?.is_safe && bankerResult.safe_sequence.length > 0 && (
            <div className="bg-white p-3 rounded-lg border border-emerald-200/80">
              <span className="text-[11px] font-semibold text-slate-500 uppercase tracking-wider block mb-1">
                Verified Safe Execution Sequence:
              </span>
              <div className="flex items-center space-x-1.5 flex-wrap">
                {bankerResult.safe_sequence.map((pid, idx) => (
                  <React.Fragment key={idx}>
                    <span className="px-2.5 py-1 bg-emerald-600 text-white rounded font-mono text-xs font-bold shadow-xs">
                      {processNames[pid] || `P${pid}`}
                    </span>
                    {idx < bankerResult.safe_sequence.length - 1 && (
                      <ArrowRight className="w-3.5 h-3.5 text-slate-400 shrink-0" />
                    )}
                  </React.Fragment>
                ))}
              </div>
            </div>
          )}
        </div>

        {/* Deadlock Detection Card */}
        <div className={`border rounded-xl p-5 shadow-xs transition-colors ${
          detectResult?.is_deadlocked
            ? 'bg-rose-50/50 border-rose-200'
            : 'bg-slate-50/80 border-slate-200'
        }`}>
          <div className="flex items-center justify-between mb-3">
            <div className="flex items-center space-x-2">
              {detectResult?.is_deadlocked ? (
                <XCircle className="w-5 h-5 text-rose-600" />
              ) : (
                <CheckCircle2 className="w-5 h-5 text-indigo-600" />
              )}
              <h3 className="text-sm font-bold text-slate-800 uppercase tracking-wide">
                Active Deadlock Detection
              </h3>
            </div>
            <span className={`px-2.5 py-1 rounded-full text-xs font-bold ${
              detectResult?.is_deadlocked
                ? 'bg-rose-100 text-rose-800'
                : 'bg-indigo-100 text-indigo-800'
            }`}>
              {detectResult?.is_deadlocked ? 'DEADLOCK CONFIRMED' : 'NO DEADLOCK'}
            </span>
          </div>

          <p className="text-xs text-slate-600 mb-3 leading-relaxed">
            {detectResult?.explanation || detectResult?.message}
          </p>

          {detectResult?.is_deadlocked && detectResult.deadlocked_processes.length > 0 && (
            <div className="bg-white p-3 rounded-lg border border-rose-200">
              <span className="text-[11px] font-semibold text-rose-700 uppercase tracking-wider block mb-1">
                Deadlocked Process Circular Cycle:
              </span>
              <div className="flex items-center space-x-2 flex-wrap">
                {detectResult.deadlocked_processes.map((pid) => (
                  <span
                    key={pid}
                    className="px-2.5 py-1 bg-rose-600 text-white rounded font-mono text-xs font-bold shadow-xs"
                  >
                    {processNames[pid] || `P${pid}`}
                  </span>
                ))}
              </div>
            </div>
          )}
        </div>
      </div>

      {/* Step-by-Step Work Vector Expansion Table (Banker Algorithm Progression) */}
      {bankerResult?.is_safe && bankerResult.steps && bankerResult.steps.length > 0 && (
        <div className="bg-white border border-slate-200 rounded-xl p-5 shadow-xs">
          <div className="flex items-center space-x-2 mb-3">
            <Activity className="w-4 h-4 text-emerald-600" />
            <h3 className="text-sm font-bold text-slate-800 uppercase tracking-wide">
              Step-by-Step Banker&apos;s Safety Work Vector Progression
            </h3>
          </div>
          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs border-collapse">
              <thead>
                <tr className="border-b border-slate-200 text-slate-500 font-semibold bg-slate-50/70">
                  <th className="py-2.5 px-3">Step</th>
                  <th className="py-2.5 px-3">Allocated Process</th>
                  <th className="py-2.5 px-3">Process Need Vector</th>
                  <th className="py-2.5 px-3">Satisfies Work &ge; Need?</th>
                  <th className="py-2.5 px-3">Work Vector After Release (Work + Allocation)</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100">
                {bankerResult.steps.map((st, i) => (
                  <tr key={i} className="hover:bg-slate-50/50">
                    <td className="py-2.5 px-3 font-semibold text-slate-700">#{st.step + 1}</td>
                    <td className="py-2.5 px-3">
                      <span className="px-2 py-0.5 bg-indigo-100 text-indigo-700 font-mono font-bold rounded">
                        {st.process_name}
                      </span>
                    </td>
                    <td className="py-2.5 px-3 font-mono text-slate-600">
                      [{needMatrix[st.process_idx]?.slice(0, numResources).join(', ')}]
                    </td>
                    <td className="py-2.5 px-3">
                      <span className="inline-flex items-center text-emerald-600 font-medium space-x-1">
                        <CheckCircle2 className="w-3.5 h-3.5" />
                        <span>Satisfied</span>
                      </span>
                    </td>
                    <td className="py-2.5 px-3 font-mono font-bold text-slate-800">
                      [{st.work.join(', ')}]
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* Interactive Matrix Configuration Section */}
      <div className="bg-white border border-slate-200 rounded-xl p-6 shadow-xs space-y-6">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 border-b border-slate-200 pb-4">
          <div>
            <h3 className="text-base font-bold text-slate-800">
              System Resource & Allocation Matrices
            </h3>
            <p className="text-xs text-slate-500">
              Configure processes ($n$), resource types ($m$), available vector, and per-process allocations.
            </p>
          </div>
          <div className="flex items-center space-x-2">
            <button
              onClick={handleAddProcess}
              disabled={numProcesses >= 10}
              className="px-2.5 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-medium rounded-lg transition-colors flex items-center space-x-1 cursor-pointer disabled:opacity-40"
            >
              <Plus className="w-3.5 h-3.5" />
              <span>Add Process</span>
            </button>
            <button
              onClick={handleAddResource}
              disabled={numResources >= 6}
              className="px-2.5 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-medium rounded-lg transition-colors flex items-center space-x-1 cursor-pointer disabled:opacity-40"
            >
              <Plus className="w-3.5 h-3.5" />
              <span>Add Resource Type</span>
            </button>
            <button
              onClick={() => handleRemoveResource(numResources - 1)}
              disabled={numResources <= 1}
              className="px-2.5 py-1.5 bg-slate-100 hover:bg-slate-200 text-slate-700 text-xs font-medium rounded-lg transition-colors flex items-center space-x-1 cursor-pointer disabled:opacity-40"
            >
              <Trash2 className="w-3.5 h-3.5" />
              <span>Remove Resource Type</span>
            </button>
          </div>
        </div>

        {/* Available Vector Input */}
        <div>
          <label className="text-xs font-bold text-slate-700 uppercase tracking-wider block mb-2">
            Available Vector ($1 \times m$ Free System Resources)
          </label>
          <div className="flex items-center space-x-3 overflow-x-auto pb-2">
            {Array.from({ length: numResources }).map((_, j) => (
              <div key={j} className="flex flex-col items-center bg-slate-50 border border-slate-200 rounded-lg p-2 min-w-[70px]">
                <span className="text-[11px] font-mono font-bold text-slate-600 mb-1">
                  {resourceNames[j] || `R${j}`}
                </span>
                <input
                  type="number"
                  min="0"
                  max="99"
                  value={available[j] ?? 0}
                  onChange={(e) => handleAvailableChange(j, parseInt(e.target.value, 10) || 0)}
                  className="w-12 text-center text-sm font-mono font-bold border border-slate-300 rounded px-1 py-1 focus:ring-1 focus:ring-indigo-500 outline-hidden"
                />
              </div>
            ))}
          </div>
        </div>

        {/* Matrix Comparison Table: Allocation vs Max Claim vs Derived Need */}
        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs border-collapse">
            <thead>
              <tr className="border-b border-slate-200 text-slate-600 bg-slate-50/80">
                <th className="py-2.5 px-3">Process</th>
                <th className="py-2.5 px-3 text-center" colSpan={numResources}>
                  Allocation Matrix ($n \times m$)
                </th>
                <th className="py-2.5 px-3 text-center" colSpan={numResources}>
                  Max Claim Matrix ($n \times m$)
                </th>
                <th className="py-2.5 px-3 text-center" colSpan={numResources}>
                  Derived Need Matrix (Max - Alloc)
                </th>
                <th className="py-2.5 px-3 text-right">Actions</th>
              </tr>
              <tr className="border-b border-slate-200 text-[11px] font-mono text-slate-400">
                <th className="py-1 px-3"></th>
                {Array.from({ length: numResources }).map((_, j) => (
                  <th key={`a-h-${j}`} className="py-1 px-2 text-center">{resourceNames[j]}</th>
                ))}
                {Array.from({ length: numResources }).map((_, j) => (
                  <th key={`m-h-${j}`} className="py-1 px-2 text-center">{resourceNames[j]}</th>
                ))}
                {Array.from({ length: numResources }).map((_, j) => (
                  <th key={`n-h-${j}`} className="py-1 px-2 text-center">{resourceNames[j]}</th>
                ))}
                <th className="py-1 px-3"></th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-100 font-mono">
              {Array.from({ length: numProcesses }).map((_, i) => (
                <tr key={i} className="hover:bg-slate-50/50">
                  <td className="py-2 px-3 font-sans font-semibold text-slate-800">
                    {processNames[i] || `P${i}`}
                  </td>

                  {/* Allocation inputs */}
                  {Array.from({ length: numResources }).map((_, j) => (
                    <td key={`alloc-${i}-${j}`} className="py-2 px-2 text-center">
                      <input
                        type="number"
                        min="0"
                        max="99"
                        value={allocation[i]?.[j] ?? 0}
                        onChange={(e) => handleAllocationChange(i, j, parseInt(e.target.value, 10) || 0)}
                        className="w-10 text-center font-bold border border-slate-300 rounded py-0.5 focus:ring-1 focus:ring-indigo-500 outline-hidden"
                      />
                    </td>
                  ))}

                  {/* Max Claim inputs */}
                  {Array.from({ length: numResources }).map((_, j) => (
                    <td key={`max-${i}-${j}`} className="py-2 px-2 text-center">
                      <input
                        type="number"
                        min="0"
                        max="99"
                        value={maxClaim[i]?.[j] ?? 0}
                        onChange={(e) => handleMaxClaimChange(i, j, parseInt(e.target.value, 10) || 0)}
                        className="w-10 text-center font-bold border border-slate-300 rounded py-0.5 focus:ring-1 focus:ring-indigo-500 outline-hidden"
                      />
                    </td>
                  ))}

                  {/* Derived Need */}
                  {Array.from({ length: numResources }).map((_, j) => (
                    <td key={`need-${i}-${j}`} className="py-2 px-2 text-center font-bold text-indigo-700 bg-indigo-50/30">
                      {needMatrix[i]?.[j] ?? 0}
                    </td>
                  ))}

                  {/* Remove process button */}
                  <td className="py-2 px-3 text-right">
                    <button
                      onClick={() => handleRemoveProcess(i)}
                      disabled={numProcesses <= 2}
                      className="text-slate-400 hover:text-rose-600 transition-colors p-1 cursor-pointer disabled:opacity-20"
                      title="Remove process"
                    >
                      <Trash2 className="w-3.5 h-3.5" />
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>

      {/* Resource Request Simulator */}
      <div className="bg-white border border-slate-200 rounded-xl p-6 shadow-xs space-y-4">
        <div className="flex items-center space-x-2">
          <Cpu className="w-5 h-5 text-indigo-600" />
          <h3 className="text-base font-bold text-slate-800">
            Resource-Request Algorithm Simulator
          </h3>
        </div>
        <p className="text-xs text-slate-500">
          Simulate an immediate resource request by process $P_i$. The Banker&apos;s request algorithm verifies whether $Request \le Need_i$, $Request \le Available$, and whether granting the request keeps the system in a safe state.
        </p>

        <div className="flex flex-wrap items-center gap-4 bg-slate-50 p-4 rounded-lg border border-slate-200">
          <div>
            <label className="text-[11px] font-semibold text-slate-500 uppercase tracking-wider block mb-1">
              Requesting Process
            </label>
            <select
              value={requestProcessIdx}
              onChange={(e) => setRequestProcessIdx(parseInt(e.target.value, 10))}
              className="border border-slate-300 rounded-lg px-3 py-1.5 text-xs font-semibold bg-white focus:ring-1 focus:ring-indigo-500 outline-hidden"
            >
              {Array.from({ length: numProcesses }).map((_, i) => (
                <option key={i} value={i}>
                  {processNames[i] || `P${i}`}
                </option>
              ))}
            </select>
          </div>

          <div>
            <label className="text-[11px] font-semibold text-slate-500 uppercase tracking-wider block mb-1">
              Request Vector
            </label>
            <div className="flex items-center space-x-2">
              {Array.from({ length: numResources }).map((_, j) => (
                <div key={j} className="flex items-center space-x-1">
                  <span className="text-[10px] font-mono text-slate-400">{resourceNames[j]}:</span>
                  <input
                    type="number"
                    min="0"
                    max="99"
                    value={requestVector[j] ?? 0}
                    onChange={(e) => {
                      const val = parseInt(e.target.value, 10) || 0;
                      setRequestVector(requestVector.map((c, idx) => (idx === j ? val : c)));
                    }}
                    className="w-10 text-center font-mono font-bold text-xs border border-slate-300 rounded py-1 bg-white focus:ring-1 focus:ring-indigo-500 outline-hidden"
                  />
                </div>
              ))}
            </div>
          </div>

          <div className="self-end">
            <button
              onClick={handleTestRequest}
              className="px-4 py-1.5 bg-indigo-600 hover:bg-indigo-700 text-white text-xs font-medium rounded-lg shadow-xs transition-colors cursor-pointer"
            >
              Evaluate Request
            </button>
          </div>
        </div>

        {/* Request Result */}
        {requestResult && (
          <div className={`p-4 rounded-lg border text-xs leading-relaxed ${
            requestResult.can_grant
              ? 'bg-emerald-50 border-emerald-200 text-emerald-800'
              : 'bg-rose-50 border-rose-200 text-rose-800'
          }`}>
            <div className="flex items-center space-x-2 font-bold mb-1">
              {requestResult.can_grant ? (
                <CheckCircle2 className="w-4 h-4 text-emerald-600" />
              ) : (
                <XCircle className="w-4 h-4 text-rose-600" />
              )}
              <span>
                {requestResult.can_grant
                  ? 'Request Granted (Safe State Maintained)'
                  : 'Request Denied / Process Must Wait'}
              </span>
            </div>
            <p>{requestResult.reason}</p>
            {requestResult.can_grant && requestResult.resulting_safe_sequence.length > 0 && (
              <div className="mt-2 text-[11px] text-slate-600">
                Resulting Safe Sequence: &lt;{requestResult.resulting_safe_sequence.map(p => processNames[p] || `P${p}`).join(' &rarr; ')}&gt;
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
};
