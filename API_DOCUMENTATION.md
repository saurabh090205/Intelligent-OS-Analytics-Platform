# REST API Documentation

## Intelligent OS Analytics and Resource Management Platform

Base URL: `http://127.0.0.1:8080/api`

All responses are encoded in `application/json; charset=utf-8` and include standard CORS headers allowing cross-origin requests.

---

## 1. Health & Platform Info

### `GET /api/health`
Returns backend service availability, version, host platform, and system uptime.

#### Response: `200 OK`
```json
{
  "status": "ok",
  "service": "Intelligent OS Analytics Engine",
  "version": "1.0.0",
  "phase": "Phase 1 - Process Monitoring & System Overview",
  "platform": "Linux (Ubuntu/Kernel)",
  "uptime_seconds": 89446,
  "timestamp": 1729853000
}
```

---

## 2. System Overview Telemetry

### `GET /api/system/overview`
Returns a unified snapshot of system processor utilization, physical memory usage, and process state counts.

#### Response: `200 OK`
```json
{
  "cpu_utilization": 24.50,
  "cpu_cores": 12,
  "cpu_model": "x86_64 Processor",
  "memory_total_kb": 16478072,
  "memory_available_kb": 2181788,
  "memory_used_kb": 14296284,
  "memory_free_kb": 2181788,
  "memory_utilization": 86.76,
  "total_processes": 356,
  "running_processes": 3,
  "sleeping_processes": 353,
  "stopped_processes": 0,
  "zombie_processes": 0,
  "uptime_seconds": 89453,
  "timestamp": 1729853005
}
```

---

## 3. CPU Telemetry

### `GET /api/system/cpu`
Returns detailed CPU utilization calculated from consecutive `/proc/stat` jiffy reads, including core count, clock frequency (user HZ), and per-core utilization.

#### Response: `200 OK`
```json
{
  "utilization": 24.50,
  "cores": 12,
  "user_hz": 100,
  "total_jiffies": 1073079375000,
  "idle_jiffies": 851400937500,
  "model_name": "x86_64 Processor",
  "per_core": [
    {
      "core_id": 0,
      "utilization_pct": 22.10
    },
    {
      "core_id": 1,
      "utilization_pct": 26.80
    }
  ],
  "timestamp": 1729853005
}
```

---

## 4. Memory Telemetry

### `GET /api/system/memory`
Returns physical RAM and swap space utilization parsed directly from `/proc/meminfo`.

#### Response: `200 OK`
```json
{
  "total_kb": 16478072,
  "available_kb": 2173548,
  "used_kb": 14304524,
  "free_kb": 2173548,
  "utilization_pct": 86.81,
  "buffers_kb": 345120,
  "cached_kb": 4120890,
  "swap_total_kb": 9457360,
  "swap_free_kb": 6892940,
  "swap_used_kb": 2564420,
  "timestamp": 1729853005
}
```

---

## 5. Process Monitoring

### `GET /api/processes`
Returns a list of all accessible running operating system processes parsed from `/proc`.

#### Response: `200 OK`
```json
{
  "total": 356,
  "count": 356,
  "running": 3,
  "sleeping": 353,
  "stopped": 0,
  "zombie": 0,
  "uptime_seconds": 89453,
  "processes": [
    {
      "pid": 1,
      "ppid": 0,
      "name": "systemd",
      "state": "S",
      "state_full": "Sleeping",
      "cpu_pct": 0.05,
      "mem_kb": 14200,
      "vsz_kb": 168400,
      "mem_pct": 0.08,
      "threads": 1,
      "priority": 20,
      "nice": 0
    }
  ],
  "timestamp": 1729853005
}
```

---

## 6. Single Process Telemetry

### `GET /api/processes/:pid`
Returns deep telemetry and scheduler statistics for a single process.

#### Parameters
- `:pid` (Required, numeric integer): The Process ID.

#### Response: `200 OK`
```json
{
  "pid": 1024,
  "ppid": 1,
  "name": "code",
  "state": "S",
  "state_full": "Sleeping",
  "cpu_pct": 1.45,
  "mem_rss_kb": 154200,
  "mem_vsz_kb": 894200,
  "mem_pct": 0.93,
  "threads": 16,
  "priority": 20,
  "nice": 0,
  "cmdline": "/usr/share/code/code --no-sandbox",
  "utime_ticks": 450,
  "stime_ticks": 120,
  "start_time_seconds": 1240,
  "timestamp": 1729853005
}
```

#### Response: `404 Not Found` (Process does not exist)
```json
{
  "status": 404,
  "error": "Process with PID 9999999 not found or terminated",
  "timestamp": 1729853005
}
```

#### Response: `400 Bad Request` (Invalid PID parameter)
```json
{
  "status": 400,
  "error": "Invalid PID format: must be numeric integer",
  "timestamp": 1729853005
}
```

---

## 7. CPU Scheduling Simulation & Recommendation (Phase 2)

### `POST /api/scheduling/simulate`
Runs complete algorithmic simulation for 5 CPU scheduling algorithms (FCFS, SJF non-preemptive, SRTF preemptive, Priority, and Round Robin) entirely in C, generating per-process metrics, timeline Gantt chart segments, and an explainable scheduling recommendation.

#### Request Headers
`Content-Type: application/json`

#### Request Body
```json
{
  "processes": [
    { "pid": 1, "name": "P1", "arrival_time": 0, "burst_time": 8, "priority": 3 },
    { "pid": 2, "name": "P2", "arrival_time": 1, "burst_time": 4, "priority": 1 },
    { "pid": 3, "name": "P3", "arrival_time": 2, "burst_time": 9, "priority": 4 }
  ],
  "quantum": 3
}
```

#### Response: `200 OK`
```json
{
  "quantum": 3,
  "process_count": 3,
  "recommended_algo_id": 2,
  "recommended_algo_name": "Shortest Remaining Time First (SRTF)",
  "recommendation_reason": "Shortest Remaining Time First (SRTF) achieved the lowest average waiting time (4.67 ms) and turnaround time (11.67 ms). Its preemptive policy effectively avoids the convoy effect caused by long CPU bursts.",
  "burst_variance": 4.67,
  "has_staggered_arrivals": true,
  "algorithms": {
    "fcfs": {
      "algo_id": 0,
      "algo_name": "First Come First Serve (FCFS)",
      "avg_waiting_time": 5.67,
      "avg_turnaround_time": 12.67,
      "avg_response_time": 5.67,
      "cpu_utilization_pct": 100.0,
      "throughput": 0.1429,
      "total_time": 21,
      "idle_time": 0,
      "processes": [
        {
          "pid": 1,
          "name": "P1",
          "arrival_time": 0,
          "burst_time": 8,
          "priority": 3,
          "start_time": 0,
          "completion_time": 8,
          "turnaround_time": 8,
          "waiting_time": 0,
          "response_time": 0
        }
      ],
      "gantt": [
        {
          "pid": 1,
          "name": "P1",
          "start_time": 0,
          "end_time": 8,
          "is_idle": false
        }
      ]
    },
    "sjf": { /* ... */ },
    "srtf": { /* ... */ },
    "priority": { /* ... */ },
    "round_robin": { /* ... */ }
  },
  "timestamp": 1729853100
}
```

### `GET /api/scheduling/examples`
Returns pre-configured textbook workload scenarios (Convoy effect, interactive time-sharing, priority mix) for demonstration and testing.

---

## 8. Memory Usage Prediction & Trend Forecasting (Phase 2)

### `GET /api/memory/forecast?horizon=60&window=10&threshold=85.0`
Computes statistical Simple Moving Average (SMA) and Simple Linear Regression on real OS sampled telemetry collected by the backend.

#### Query Parameters:
- `horizon` (int, default: 60): Forecast horizon in seconds (15 to 300).
- `window` (int, default: 10): Moving average window size in samples (5 to 30).
- `threshold` (double, default: 85.0): Alert threshold percentage (50.0 to 95.0).

#### Response: `200 OK`
```json
{
  "trend_id": 1,
  "trend_label": "Increasing (Memory Growth Detected)",
  "slope_pct_per_min": 1.25,
  "r_squared": 0.8412,
  "current_utilization_pct": 82.30,
  "predicted_sma_util_pct": 82.50,
  "predicted_reg_util_pct": 86.80,
  "current_available_kb": 2916000,
  "predicted_available_kb": 2174000,
  "total_ram_kb": 16478072,
  "forecast_horizon_seconds": 60,
  "sample_count": 60,
  "threshold_exceeded": true,
  "threshold_pct": 85.0,
  "seconds_to_exhaustion": 450,
  "warning_message": "WARNING: Memory utilization is projected to reach 86.8% (exceeding 85% safety threshold).",
  "limitation_explanation": "Statistical moving average and linear regression extrapolate historical measurements. Sudden process launches, dynamic page cache evictions, or memory drops cannot be predicted.",
  "forecast_points": [
    {
      "timestamp": 1729853107,
      "offset_seconds": 7,
      "predicted_util_pct": 82.85,
      "estimated_used_kb": 13652000,
      "estimated_available_kb": 2826072
    }
  ],
  "timestamp": 1729853100
}
```

### `GET /api/memory/history`
Returns timestamped historical memory utilization samples stored in the backend rolling buffer.

---

## 9. Page Replacement Simulation (Phase 2)

### `POST /api/page-replacement/simulate`
Executes frame-by-frame memory simulation for FIFO, LRU, and Optimal page replacement algorithms, generating exact step-by-step matrix tables and performance comparisons.

#### Request Body
```json
{
  "reference_string": [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1],
  "frame_count": 3
}
```

#### Response: `200 OK`
```json
{
  "frames": 3,
  "reference_count": 20,
  "best_algo_id": 2,
  "best_algo_name": "Optimal Page Replacement (OPT)",
  "min_faults": 9,
  "comparison_summary": "Optimal page replacement generated the minimum page faults (9 faults, 55.0% hit ratio) serving as theoretical lower bound. LRU achieved 12 faults (40.0% hits), outperforming FIFO (15 faults, 25.0% hits).",
  "algorithms": {
    "fifo": {
      "algo_id": 0,
      "algo_name": "First-In First-Out (FIFO)",
      "frame_capacity": 3,
      "total_references": 20,
      "hit_count": 5,
      "fault_count": 15,
      "hit_ratio": 0.2500,
      "fault_ratio": 0.7500,
      "steps": [
        {
          "step": 0,
          "page": 7,
          "is_hit": false,
          "victim_page": -1,
          "replaced_frame_idx": 0,
          "frames": [7, -1, -1]
        }
      ]
    },
    "lru": { /* ... */ },
    "optimal": { /* ... */ }
  },
  "timestamp": 1729853100
}
```

### `GET /api/page-replacement/examples`
Returns reference strings including Silberschatz textbook example, Belady's Anomaly demo (9 faults with 3 frames vs 10 faults with 4 frames in FIFO), and high temporal locality workloads.

---

## 10. Deadlock Detection & Avoidance (Phase 3)

### `POST /api/deadlock/banker`
Evaluates system safety using Dijkstra's Banker's Algorithm. Computes the Need matrix ($Need_{i,j} = Max_{i,j} - Alloc_{i,j}$), determines whether the state is safe or unsafe, and outputs the safe sequence with intermediate Work vectors.

#### Request Body
```json
{
  "available": [3, 3, 2],
  "allocation": [
    [0, 1, 0],
    [2, 0, 0],
    [3, 0, 2],
    [2, 1, 1],
    [0, 0, 2]
  ],
  "max_claim": [
    [7, 5, 3],
    [3, 2, 2],
    [9, 0, 2],
    [2, 2, 2],
    [4, 3, 3]
  ]
}
```
*(Alternatively, pass `{"example_id": "silberschatz_safe"}` or `{"example_id": "unsafe_state"}`)*

#### Response: `200 OK`
```json
{
  "is_safe": true,
  "status_message": "System is in a SAFE state.",
  "explanation": "A valid execution sequence exists: <P1 -> P3 -> P0 -> P2 -> P4>. All processes can satisfy their maximum resource claims and terminate without entering deadlock.",
  "num_processes": 5,
  "num_resources": 3,
  "safe_sequence": [1, 3, 0, 2, 4],
  "steps": [
    { "step": 0, "process_idx": 1, "process_name": "P1", "work": [5, 3, 2] }
  ],
  "need": [[7, 4, 3], [1, 2, 2], [6, 0, 0], [0, 1, 1], [4, 3, 1]],
  "available": [3, 3, 2],
  "timestamp": 1792943258
}
```

### `POST /api/deadlock/request`
Evaluates an immediate resource request by process $P_i$. Checks if $Request \le Need_i$, $Request \le Available$, and whether granting the request keeps the system in a safe state.

#### Request Body
```json
{
  "example_id": "silberschatz_safe",
  "process_idx": 1,
  "request": [1, 0, 2]
}
```

#### Response: `200 OK`
```json
{
  "process_idx": 1,
  "can_grant": true,
  "would_be_safe": true,
  "reason": "Request can be GRANTED immediately. Resulting system state remains in a SAFE state.",
  "resulting_safe_sequence": [1, 3, 0, 2, 4],
  "timestamp": 1792943269
}
```

### `POST /api/deadlock/detect`
Performs immediate deadlock detection using the resource-allocation model with circular wait detection.

#### Response: `200 OK`
```json
{
  "is_deadlocked": true,
  "deadlocked_count": 2,
  "message": "DEADLOCK DETECTED involving 2 processes.",
  "explanation": "A circular wait or unfulfillable request cycle exists among processes: [Proc-A Proc-B ].",
  "deadlocked_processes": [0, 1],
  "timestamp": 1792943274
}
```

### `GET /api/deadlock/examples`
Returns three standard pre-configured system states: Silberschatz Safe State, Unsafe State (Avoidance Warning), and Active Deadlock Cycle.

---

## 11. Operating System Log Analysis (Phase 3)

### `POST /api/logs/analyze`
Parses and categorizes multi-format operating system log streams (Linux Syslog RFC 3164/5424, ISO 8601, and Windows Event logs). Identifies recurring error patterns, extracts severity, detects error bursts, and safely isolates unformatted lines.

#### Request Body
```json
{
  "content": "Oct 25 14:12:44 host kernel: Out of memory: Kill process 8912\nOct 25 14:14:02 host systemd: nginx.service failed with core-dump",
  "min_severity": "ALL",
  "category": -1,
  "keyword": ""
}
```
*(Alternatively, provide `"file_path": "C:\\path\\to\\logfile.log"`)*

#### Response: `200 OK`
```json
{
  "total_lines_read": 2,
  "parsed_entries_count": 2,
  "unrecognized_lines_count": 0,
  "error_count": 2,
  "warning_count": 0,
  "info_count": 0,
  "debug_count": 0,
  "burst_detected": false,
  "burst_description": "Log stream frequency is nominal. No cascading error bursts detected.",
  "category_counts": {
    "general": 1,
    "memory_oom": 1,
    "kernel_panic": 0,
    "storage_disk": 0,
    "auth_security": 0,
    "network": 0,
    "service_crash": 0
  },
  "recurring_patterns": [
    { "pattern": "Out-Of-Memory (OOM Killer)", "count": 1, "severity": "CRITICAL" }
  ],
  "entries": [
    {
      "line": 1,
      "timestamp": "Oct 25 14:12:44",
      "source": "kernel",
      "severity": "CRITICAL",
      "category": "Memory / OOM",
      "message": "Out of memory: Kill process 8912",
      "is_supported_format": true
    }
  ],
  "timestamp": 1792943280
}
```

### `POST /api/logs/analyze` (File Path Mode)
Alternatively, `file_path` can specify an existing log file on disk.

```json
{
  "file_path": "backend/tests/sample.log"
}
```

#### Security Restrictions on `file_path`:
The backend C engine strictly enforces security boundaries on local file analysis:
- **Path Traversal Defense**: Any path containing `..` is denied with `403 Forbidden`.
- **System Target Restrictions**: Access to operating system directories or credentials (`system32`, `syswow64`, `config/sam`, `config/system`, `config/security`, `.ssh`, `.env`, `ntuser.dat`, `/etc/shadow`, `/etc/sudoers`) is denied with `403 Forbidden`.
- **Executable & Script Defense**: Files with executable or script extensions (`.exe`, `.dll`, `.sys`, `.bin`, `.dat`, `.so`, `.msi`, `.bat`, `.cmd`, `.ps1`, `.sh`) are denied with `403 Forbidden`.
- **File Size Upper Bound**: Files larger than 5 MB are rejected with `413 Payload Too Large`.

---

## 12. Advisory Resource Allocation Engine

### `GET /api/advisory/recommendations`
Synthesizes live host telemetry (CPU, Memory, Process Table) and analytical models (Linear Regression Memory Forecast, Page Replacement Benchmarks) into actionable systems guidance. Includes measured vs threshold metrics, confidence %, and the formal safety disclaimer.

#### Response: `200 OK`
```json
{
  "overview": {
    "cpu_pressure": "Nominal",
    "memory_pressure": "Critical",
    "process_pressure": "Nominal",
    "paging_pressure": "Nominal",
    "overall_health": "Critical Pressure",
    "total_recommendations": 4,
    "warning_count": 1,
    "critical_count": 1
  },
  "items": [
    {
      "id": "mem_critical",
      "category_id": 1,
      "category": "Memory Management",
      "severity_id": 2,
      "severity": "CRITICAL",
      "title": "High Physical Memory Utilization Alert",
      "rationale": "Physical memory usage is 75.6% with upward linear trajectory (+11.13%/min). Estimated available RAM is 3923 MB.",
      "recommended_action": "Audit memory-intensive resident sets. Release idle buffers and monitor for potential Out-Of-Memory (OOM) killer events.",
      "confidence_pct": 92,
      "is_pure_telemetry": true,
      "metric_name": "RAM Utilization",
      "measured_value": "75.6%",
      "threshold_value": "85.0%"
    }
  ],
  "disclaimer": "ADVISORY NOTICE: All recommendations are analytical guidance intended for systems administrators. The platform adheres to strict safety boundaries: it does NOT automatically terminate processes, alter process nice levels, or mutate kernel sysctl configurations. Measured telemetry is collected directly from host OS APIs; forecasts and benchmarks represent statistical models.",
  "timestamp": 1792943029
}
```

---

## 13. HTTP Status Codes

| Code | Status | Description |
| :--- | :--- | :--- |
| `200` | OK | Request processed successfully. |
| `204` | No Content | OPTIONS preflight request accepted. |
| `400` | Bad Request | Parameter syntax error, missing body, invalid matrix dimensions, or allocation > max claim. |
| `403` | Forbidden | Security violation: path traversal (`..`), sensitive system file, or prohibited binary extension. |
| `404` | Not Found | Route endpoint or requested PID does not exist. |
| `405` | Method Not Allowed | Request used unsupported HTTP method. |
| `413` | Payload Too Large | Analyzed file exceeds maximum allowed threshold (5 MB). |
| `500` | Internal Server Error | Algorithm calculation error or system counter read failure. |



