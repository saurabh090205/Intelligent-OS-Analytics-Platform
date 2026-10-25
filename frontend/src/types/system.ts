export interface ApiHealth {
  status: string;
  service: string;
  version: string;
  phase: string;
  platform: string;
  uptime_seconds: number;
  timestamp: number;
}

export interface SystemOverview {
  cpu_utilization: number;
  cpu_cores: number;
  cpu_model: string;
  memory_total_kb: number;
  memory_available_kb: number;
  memory_used_kb: number;
  memory_free_kb: number;
  memory_utilization: number;
  total_processes: number;
  running_processes: number;
  sleeping_processes: number;
  stopped_processes: number;
  zombie_processes: number;
  uptime_seconds: number;
  timestamp: number;
}

export interface CpuCore {
  core_id: number;
  utilization_pct: number;
}

export interface CpuStats {
  utilization: number;
  cores: number;
  user_hz: number;
  total_jiffies: number;
  idle_jiffies: number;
  model_name: string;
  per_core: CpuCore[];
  timestamp: number;
}

export interface MemoryStats {
  total_kb: number;
  available_kb: number;
  used_kb: number;
  free_kb: number;
  utilization_pct: number;
  buffers_kb: number;
  cached_kb: number;
  swap_total_kb: number;
  swap_free_kb: number;
  swap_used_kb: number;
  timestamp: number;
}

export interface ProcessSummary {
  pid: number;
  ppid: number;
  name: string;
  state: string;
  state_full: string;
  cpu_pct: number;
  mem_kb: number;
  vsz_kb: number;
  mem_pct: number;
  threads: number;
  priority: number;
  nice: number;
}

export interface ProcessTableResponse {
  total: number;
  count: number;
  running: number;
  sleeping: number;
  stopped: number;
  zombie: number;
  uptime_seconds: number;
  processes: ProcessSummary[];
  timestamp: number;
}

export interface ProcessDetail {
  pid: number;
  ppid: number;
  name: string;
  state: string;
  state_full: string;
  cpu_pct: number;
  mem_rss_kb: number;
  mem_vsz_kb: number;
  mem_pct: number;
  threads: number;
  priority: number;
  nice: number;
  cmdline: string;
  utime_ticks: number;
  stime_ticks: number;
  start_time_seconds: number;
  timestamp: number;
}

export interface HistoryPoint {
  time: string;
  timestamp: number;
  cpu: number;
  memory: number;
}

/* ==================== Phase 2: CPU Scheduling Types ==================== */

export interface SchedProcessInput {
  pid: number;
  name: string;
  arrival_time: number;
  burst_time: number;
  priority: number;
}

export interface SchedProcessOutput {
  pid: number;
  name: string;
  arrival_time: number;
  burst_time: number;
  priority: number;
  start_time: number;
  completion_time: number;
  turnaround_time: number;
  waiting_time: number;
  response_time: number;
}

export interface GanttSegment {
  pid: number;
  name: string;
  start_time: number;
  end_time: number;
  is_idle: boolean;
}

export interface SchedResult {
  algo_id: number;
  algo_name: string;
  avg_waiting_time: number;
  avg_turnaround_time: number;
  avg_response_time: number;
  cpu_utilization_pct: number;
  throughput: number;
  total_time: number;
  idle_time: number;
  processes: SchedProcessOutput[];
  gantt: GanttSegment[];
}

export interface SchedAlgorithms {
  fcfs: SchedResult;
  sjf: SchedResult;
  srtf: SchedResult;
  priority: SchedResult;
  round_robin: SchedResult;
}

export interface SchedSimulationResponse {
  quantum: number;
  process_count: number;
  recommended_algo_id: number;
  recommended_algo_name: string;
  recommendation_reason: string;
  burst_variance: number;
  has_staggered_arrivals: boolean;
  algorithms: SchedAlgorithms;
  timestamp: number;
}

export interface SchedExample {
  id: string;
  title: string;
  description: string;
  quantum: number;
  processes: SchedProcessInput[];
}

export interface SchedExamplesResponse {
  examples: SchedExample[];
}

/* ==================== Phase 2: Memory Prediction Types ==================== */

export interface ForecastPoint {
  timestamp: number;
  offset_seconds: number;
  predicted_util_pct: number;
  estimated_used_kb: number;
  estimated_available_kb: number;
}

export interface MemoryForecastResponse {
  trend_id: number;
  trend_label: string;
  slope_pct_per_min: number;
  r_squared: number;
  current_utilization_pct: number;
  predicted_sma_util_pct: number;
  predicted_reg_util_pct: number;
  current_available_kb: number;
  predicted_available_kb: number;
  total_ram_kb: number;
  forecast_horizon_seconds: number;
  sample_count: number;
  threshold_exceeded: boolean;
  threshold_pct: number;
  seconds_to_exhaustion: number;
  warning_message: string;
  limitation_explanation: string;
  forecast_points: ForecastPoint[];
  timestamp: number;
}

export interface MemoryHistorySample {
  timestamp: number;
  utilization_pct: number;
  used_kb: number;
  available_kb: number;
  total_kb: number;
}

export interface MemoryHistoryResponse {
  total_recorded: number;
  sample_count: number;
  interval_seconds: number;
  samples: MemoryHistorySample[];
}

/* ==================== Phase 2: Page Replacement Types ==================== */

export interface PageStep {
  step: number;
  page: number;
  is_hit: boolean;
  victim_page: number;
  replaced_frame_idx: number;
  frames: number[];
}

export interface PageSimResult {
  algo_id: number;
  algo_name: string;
  frame_capacity: number;
  total_references: number;
  hit_count: number;
  fault_count: number;
  hit_ratio: number;
  fault_ratio: number;
  steps: PageStep[];
}

export interface PageAlgorithms {
  fifo: PageSimResult;
  lru: PageSimResult;
  optimal: PageSimResult;
}

export interface PageSimulationResponse {
  frames: number;
  reference_count: number;
  best_algo_id: number;
  best_algo_name: string;
  min_faults: number;
  comparison_summary: string;
  algorithms: PageAlgorithms;
  timestamp: number;
}

export interface PageExample {
  id: string;
  title: string;
  description: string;
  frames: number;
  references: number[];
}

export interface PageExamplesResponse {
  examples: PageExample[];
}

/* ==================== Phase 3: Deadlock Management Types ==================== */

export interface DeadlockState {
  num_processes: number;
  num_resources: number;
  available: number[];
  allocation: number[][];
  max_claim: number[][];
  need?: number[][];
  processes?: string[];
  resources?: string[];
}

export interface BankerStep {
  step: number;
  process_idx: number;
  process_name: string;
  work: number[];
}

export interface BankerResult {
  is_safe: boolean;
  status_message: string;
  explanation: string;
  num_processes: number;
  num_resources: number;
  safe_sequence: number[];
  steps: BankerStep[];
  need: number[][];
  available: number[];
  timestamp: number;
}

export interface DeadlockRequestResult {
  process_idx: number;
  can_grant: boolean;
  would_be_safe: boolean;
  reason: string;
  resulting_safe_sequence: number[];
  timestamp: number;
}

export interface DeadlockDetectionResult {
  is_deadlocked: boolean;
  deadlocked_count: number;
  message: string;
  explanation: string;
  deadlocked_processes: number[];
  timestamp: number;
}

export interface DeadlockExample {
  id: string;
  title: string;
  description: string;
  num_processes: number;
  num_resources: number;
  processes: string[];
  resources: string[];
  available: number[];
  allocation: number[][];
  max_claim: number[][];
}

export interface DeadlockExamplesResponse {
  examples: DeadlockExample[];
}

/* ==================== Phase 3: OS Log Analysis Types ==================== */

export interface LogEntry {
  line: number;
  timestamp: string;
  source: string;
  severity: string;
  category: string;
  message: string;
  is_supported_format: boolean;
}

export interface RecurringPattern {
  pattern: string;
  count: number;
  severity: string;
}

export interface CategoryCounts {
  general: number;
  memory_oom: number;
  kernel_panic: number;
  storage_disk: number;
  auth_security: number;
  network: number;
  service_crash: number;
}

export interface LogAnalysisResponse {
  total_lines_read: number;
  parsed_entries_count: number;
  unrecognized_lines_count: number;
  error_count: number;
  warning_count: number;
  info_count: number;
  debug_count: number;
  burst_detected: boolean;
  burst_description: string;
  category_counts: CategoryCounts;
  recurring_patterns: RecurringPattern[];
  entries: LogEntry[];
  timestamp: number;
}

export interface LogSampleResponse {
  sample_id: string;
  content: string;
  timestamp: number;
}

/* ==================== Phase 3: Resource Advisory Types ==================== */

export interface AdvisoryOverview {
  cpu_pressure: string;
  memory_pressure: string;
  process_pressure: string;
  paging_pressure: string;
  overall_health: string;
  total_recommendations: number;
  warning_count: number;
  critical_count: number;
}

export interface AdvisoryItem {
  id: string;
  category_id: number;
  category: string;
  severity_id: number;
  severity: 'INFO' | 'WARNING' | 'CRITICAL';
  title: string;
  rationale: string;
  recommended_action: string;
  confidence_pct: number;
  is_pure_telemetry: boolean;
  metric_name: string;
  measured_value: string;
  threshold_value: string;
}

export interface ResourceAdvisoryResponse {
  overview: AdvisoryOverview;
  items: AdvisoryItem[];
  disclaimer: string;
  timestamp: number;
}
