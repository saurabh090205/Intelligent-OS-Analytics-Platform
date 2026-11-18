import type {
  ApiHealth,
  SystemOverview,
  CpuStats,
  MemoryStats,
  ProcessTableResponse,
  ProcessDetail,
  SchedProcessInput,
  SchedSimulationResponse,
  SchedExamplesResponse,
  MemoryForecastResponse,
  MemoryHistoryResponse,
  PageSimulationResponse,
  PageExamplesResponse,
  DeadlockState,
  BankerResult,
  DeadlockRequestResult,
  DeadlockDetectionResult,
  DeadlockExamplesResponse,
  LogAnalysisResponse,
  LogSampleResponse,
  ResourceAdvisoryResponse
} from '../types/system';

const API_BASE = 'http://127.0.0.1:8080/api';

async function request<T>(endpoint: string, options?: RequestInit): Promise<T> {
  const url = `${API_BASE}${endpoint}`;
  try {
    const res = await fetch(url, {
      ...options,
      headers: {
        'Accept': 'application/json',
        ...(options?.headers || {}),
      },
    });

    if (!res.ok) {
      let errMsg = `HTTP ${res.status} ${res.statusText}`;
      try {
        const errorJson = await res.json();
        if (errorJson && errorJson.error) {
          errMsg = errorJson.error;
        }
      } catch {
        // use fallback errMsg
      }
      throw new Error(errMsg);
    }

    return await res.json() as T;
  } catch (err: unknown) {
    if (err instanceof Error) {
      throw err;
    }
    throw new Error('Network error connecting to backend service');
  }
}

export const api = {
  getHealth: () => request<ApiHealth>('/health'),
  getOverview: () => request<SystemOverview>('/system/overview'),
  getCpuStats: () => request<CpuStats>('/system/cpu'),
  getMemoryStats: () => request<MemoryStats>('/system/memory'),
  getProcesses: () => request<ProcessTableResponse>('/processes'),
  getProcessDetail: (pid: number) => request<ProcessDetail>(`/processes/${pid}`),

  /* CPU Scheduling */
  simulateScheduling: (payload: { processes: SchedProcessInput[]; quantum: number }) =>
    request<SchedSimulationResponse>('/scheduling/simulate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    }),
  getSchedulingExamples: () => request<SchedExamplesResponse>('/scheduling/examples'),

  /* Memory Prediction */
  getMemoryForecast: (horizon: number = 60, window: number = 10, threshold: number = 85.0) =>
    request<MemoryForecastResponse>(`/memory/forecast?horizon=${horizon}&window=${window}&threshold=${threshold}`),
  getMemoryHistory: () => request<MemoryHistoryResponse>('/memory/history'),

  /* Page Replacement */
  simulatePageReplacement: (payload: { reference_string: number[]; frame_count: number }) =>
    request<PageSimulationResponse>('/page-replacement/simulate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    }),
  getPageReplacementExamples: () => request<PageExamplesResponse>('/page-replacement/examples'),

  /* Deadlock Detection & Avoidance */
  analyzeBanker: (payload: Partial<DeadlockState> & { example_id?: string }) =>
    request<BankerResult>('/deadlock/banker', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    }),
  checkDeadlockRequest: (payload: { process_idx: number; request: number[] } & Partial<DeadlockState> & { example_id?: string }) =>
    request<DeadlockRequestResult>('/deadlock/request', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    }),
  detectDeadlock: (payload: Partial<DeadlockState> & { example_id?: string }) =>
    request<DeadlockDetectionResult>('/deadlock/detect', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    }),
  getDeadlockExamples: () => request<DeadlockExamplesResponse>('/deadlock/examples'),

  /* OS Log Analysis */
  analyzeLogs: (payload: { content?: string; log_text?: string; file_path?: string; min_severity?: string; category?: number; keyword?: string }) =>
    request<LogAnalysisResponse>('/logs/analyze', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    }),
  getLogSample: (id?: string) => request<LogSampleResponse>('/logs/samples' + (id ? `?id=${id}` : '')),

  /* Resource Advisory */
  getResourceAdvisory: () => request<ResourceAdvisoryResponse>('/advisory/recommendations'),
};

