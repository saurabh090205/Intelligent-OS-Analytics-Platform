# Phase 4 Final Project Report & Verification Summary

## Intelligent Operating System Analytics and Resource Management Platform

**Course Project**: Operating Systems  
**Development Phase**: Phase 4 — Final Integration, Testing, Security Hardening, Documentation, and Demonstration Readiness  
**Target Architecture**: C Backend (MinGW GCC / POSIX GCC) + React 19 / TypeScript / Vite Frontend  
**Status**: Completed and Fully Verified on Windows Development Environment  

---

## 1. Executive Summary

Phase 4 concludes the development of the **Intelligent Operating System Analytics and Resource Management Platform**. All four development phases are complete, fully integrated, hardened, and verified:
- **Phase 1**: Process Monitoring, CPU Telemetry, Memory & Swap Counters, Initial Dashboard.
- **Phase 2**: CPU Scheduling (FCFS, SJF, SRTF, Priority, Round Robin), Memory Forecasting (OLS Linear Regression & Moving Average), Virtual Memory Page Replacement (FIFO, LRU, Optimal, Belady's Anomaly).
- **Phase 3**: Dijkstra's Banker's Algorithm & Resource-Request Safety, Resource Allocation Graph Cycle Detection, Multi-Format OS Log Analysis, Advisory Resource Allocation Engine.
- **Phase 4**: Full Cross-Module Audit, Integrated Composite Dashboard, Security & Path-Traversal Defense, Socket Timeout Hardening, End-to-End API Integration Suite (29/29 Passing), Complete Unit Test Suite (142/142 Passing), Clean Production Build, and Project Documentation.

The platform provides a unified view of operating system health while rigorously distinguishing live kernel telemetry from deterministic algorithmic simulations and statistical forecasts.

---

## 2. Changes Made in Phase 4

1. **Integrated Overview Dashboard (`OverviewPage.tsx`)**:
   - Replaced isolated metric display with a cohesive command center.
   - Live OS Telemetry Header: 6 metric cards displaying CPU %, RAM %, Available RAM, Swap %, Total Processes, and System Uptime with an explicit `Live OS Measurements` badge.
   - Subsystem Health & Pressure Bar: Connected to `/api/advisory/recommendations`, displaying active pressure categories and highlighting the highest-priority advisory recommendation.
   - Real-Time Streaming Visualizations: Dual Recharts AreaCharts tracking CPU utilization and physical memory consumption.
   - 4 Cross-Module Analytical Cards: Interactive preview cards for CPU Scheduling, Memory Forecasting (OLS slope and $R^2$), Page Replacement benchmarks, and Deadlock safety state with an explicit `Algorithmic Models & Forecasts` badge.
   - Dynamic Navigation: Direct one-click tab switching (`onNavigateToTab`) from overview cards to deep module pages.
   - Top Resource Consumers & State Breakdown: Process state breakdown donut chart and Top 5 CPU/Memory process inspection tables with PID drill-down modals.

2. **Security & Path Traversal Hardening (`log_analyzer.c` & `routes.c`)**:
   - Implemented `log_is_safe_file_path()` in `backend/src/logs/log_analyzer.c` to protect host file systems.
   - Path Traversal Defense: Strictly rejects sequences containing `..` with HTTP `403 Forbidden`.
   - Sensitive Hive Restrictions: Rejects paths accessing Windows hives (`system32`, `syswow64`, `config/sam`, `config/system`, `config/security`, `ntuser.dat`) and Linux sensitive targets (`/etc/shadow`, `/etc/sudoers`, `/proc/kcore`) with HTTP `403 Forbidden`.
   - Executable/Script Defense: Blocks analysis of `.exe`, `.dll`, `.sys`, `.bin`, `.dat`, `.so`, `.bat`, `.cmd`, `.ps1`, `.sh`.
   - File Size Upper Bound: Rejects log files exceeding 5 MB with HTTP `413 Payload Too Large`.
   - Created verified workspace sample log at `backend/tests/sample.log` for immediate local analysis.

3. **Socket Timeout & Connection Reliability (`server.c`)**:
   - Configured non-blocking client socket timeouts (`SO_RCVTIMEO` and `SO_SNDTIMEO` set to 4000ms) on Windows and POSIX platforms to prevent client hang conditions.
   - Added immediate `fflush(stdout)` on startup banner to avoid buffering issues when executed under daemon managers.

4. **Automated End-to-End Integration Suite (`scripts/e2e_api_test.js`)**:
   - Created a standalone Node.js integration runner exercising all 8 functional suites (29 distinct tests) covering standard payloads, boundary conditions, malformed JSON, and security defenses.

---

## 3. Defects Discovered and Fixed

| # | Component | Defect Description | Root Cause | Resolution |
| :--- | :--- | :--- | :--- | :--- |
| **1** | `log_analyzer.c` | Legitimate workspace log `sample.log` was rejected as sensitive. | Initial check used `strstr(path, "sam")`, which matched `"sam"` in `"sample.log"`. | Refined rule to match Windows Security Account Manager hives specifically (`\config\sam`, `/config/sam`). |
| **2** | `routes.c` | `POST /api/page-replacement/simulate` did not recognize `frame_count`. | Payload parser looked only for `"frames"`, ignoring the frontend's `"frame_count"` key and falling back to default 3. | Updated `parse_page_payload()` to accept both `"frame_count"` and `"frames"`, and strictly validate $1 \le frames \le 16$, returning HTTP `400 Bad Request` on invalid input. |
| **3** | `routes.c` / `e2e_api_test.js` | Single PID inspection failed on Windows PID 0. | Windows PID 0 ("System Idle Process") cannot be queried via `OpenProcess`. | Validated PID filter to ensure `pid > 0` before calling `OpenProcess`; returned descriptive error if PID 0 is queried. |
| **4** | `server.c` | Server could hang indefinitely if a client opened a TCP socket and stalled before sending `\r\n\r\n`. | Sockets lacked receive timeouts, causing blocking `recv()` to wait indefinitely. | Added `SO_RCVTIMEO` and `SO_SNDTIMEO` (4 seconds) to all accepted sockets. |
| **5** | `OverviewPage.tsx` | Unused state variable warning caused potential bundle lint failure. | `moduleLoading` was declared but not rendered under TypeScript strict `noUnusedLocals`. | Connected `moduleLoading` to an active syncing status indicator in the analytics card header. |

---

## 4. Modules Verified End-to-End

All 7 core modules plus the Overview Dashboard and Settings console were verified:

1. **Process & Hardware Monitoring**:
   - Real-time CPU, RAM, and Swap counters matching Windows System Info.
   - Discovered active processes (>300), per-process RSS, CPU %, state tags, and command line inspection.
2. **CPU Scheduling**:
   - Verified FCFS, SJF, SRTF, Priority, and Round Robin.
   - Tested simultaneous arrivals, idle gaps, preemption under SRTF, and Round Robin time slicing ($Q=3$).
   - Confirmed explainable recommendation engine selects the lowest average turnaround/waiting time algorithm.
3. **Memory Analytics & Forecasting**:
   - History ring buffer records live memory data points.
   - OLS Linear Regression ($y = mx + c$) calculates slope (%/min) and $R^2$ goodness of fit.
   - Evaluates threshold projection times to $85\%$ and $95\%$ RAM capacity.
4. **Virtual Memory Page Replacement**:
   - Verified FIFO (15 faults), LRU (12 faults), and Optimal (9 faults) on standard textbook reference string `[7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1]`.
   - Proved Belady's Anomaly where FIFO page faults increase from 9 to 10 when frames increase from 3 to 4.
5. **Deadlock Analysis & Avoidance**:
   - Verified Need matrix derivation: $Need = Max - Allocation$.
   - Dijkstra's Banker's Algorithm validates safe sequence `<P1 -> P3 -> P0 -> P2 -> P4>`.
   - Resource-request safety evaluator grants valid requests and rejects unsafe allocations.
   - Confirms cycle detection on circular resource allocation graphs.
6. **Operating System Log Analysis**:
   - Parses Linux Syslog, Windows Event log format, and ISO 8601 timestamps.
   - Categorizes events (OOM, Storage, Kernel Panic, Auth, Service Crash).
   - Detects recurring patterns and rapid error bursts.
   - Enforces strict path traversal defenses.
7. **Advisory Resource Allocation Engine**:
   - Evaluates multi-subsystem pressure (CPU, RAM, Process count, Paging).
   - Generates prioritized recommendations with confidence scores.
   - Adheres strictly to the non-destructive advisory safety boundary (no automatic process killing).

---

## 5. API Integration Test Results

Executed via automated runner `scripts/e2e_api_test.js` against live C backend on `http://127.0.0.1:8080/api`:

```
====================================================
 Phase 4 End-to-End API Integration Test Suite
 Target Backend: http://127.0.0.1:8080/api
====================================================

[SUITE 1: System Health & Hardware Telemetry]
  [PASS] GET /api/health returns ok status & platform info
  [PASS] GET /api/system/overview returns bounded CPU & Memory telemetry
  [PASS] GET /api/system/cpu returns per-core metrics and jiffies
  [PASS] GET /api/system/memory returns RAM and swap counters

[SUITE 2: Process Monitoring & Detail Inspector]
  [PASS] GET /api/processes returns process table list
  [PASS] GET /api/processes/:pid returns single process detail
  [PASS] GET /api/processes/:pid with non-existent PID returns 404
  [PASS] GET /api/processes/:pid with alphabetic PID returns 400
  [PASS] GET /api/processes/:pid with negative PID returns 400

[SUITE 3: CPU Scheduling Algorithms & Recommendations]
  [PASS] GET /api/scheduling/examples returns preconfigured workloads
  [PASS] POST /api/scheduling/simulate evaluates all 5 algorithms and recommendation
  [PASS] POST /api/scheduling/simulate with malformed JSON returns 400

[SUITE 4: Memory Forecasting & History]
  [PASS] GET /api/memory/history returns recorded time-series buffer
  [PASS] GET /api/memory/forecast computes linear regression & threshold projection

[SUITE 5: Virtual Memory Page Replacement Simulation]
  [PASS] GET /api/page-replacement/examples returns Belady & textbook strings
  [PASS] POST /api/page-replacement/simulate computes FIFO, LRU, and Optimal
  [PASS] POST /api/page-replacement/simulate with invalid frame_count returns 400

[SUITE 6: Deadlock Detection & Banker's Algorithm]
  [PASS] GET /api/deadlock/examples returns standard system states
  [PASS] POST /api/deadlock/banker evaluates safe state and safe sequence
  [PASS] POST /api/deadlock/banker identifies unsafe state
  [PASS] POST /api/deadlock/request validates resource request safety
  [PASS] POST /api/deadlock/detect detects circular wait cycles

[SUITE 7: OS Log Analysis & Path Traversal Security]
  [PASS] GET /api/logs/samples returns representative log text
  [PASS] POST /api/logs/analyze parses Linux syslog & categorizes OOM/Crash
  [PASS] POST /api/logs/analyze parses Windows event format
  [PASS] POST /api/logs/analyze parses safe local log file (backend/tests/sample.log)
  [PASS] SECURITY: POST /api/logs/analyze rejects path traversal (../../Windows/System32/config/SAM) with 403
  [PASS] SECURITY: POST /api/logs/analyze rejects binary executables with 403

[SUITE 8: Advisory Resource Allocation Engine]
  [PASS] GET /api/advisory/recommendations returns multi-subsystem guidance & disclaimer

====================================================
 Integration Test Results: 29 / 29 Passed (100.0%)
 ALL ENDPOINTS & SECURITY VERIFICATIONS SUCCEEDED
====================================================
```

---

## 6. C Unit Test Results

Executed via `mingw32-make test` across all three automated test suites:

| Test Suite | Executable | Coverage / Scope | Passed / Total | Pass Rate |
| :--- | :--- | :--- | :---: | :---: |
| **Phase 1** | `test_monitoring.exe` | JSON builder, CPU differential sampling, memory counters, process table | 27 / 27 | 100.0% |
| **Phase 2** | `test_phase2.exe` | CPU Scheduling (5 algorithms), Memory Forecasting & OLS, Page Replacement | 53 / 53 | 100.0% |
| **Phase 3 & 4** | `test_phase3.exe` | Banker's algorithm, Deadlock cycle detection, Log parsing, Path Traversal Security, Advisory Engine | 62 / 62 | 100.0% |
| **Total** | **3 Test Executables** | **Complete Native C Engine** | **142 / 142** | **100.0%** |

---

## 7. Frontend Build Results

Production build executed via `npm run build`:

```
> frontend@0.0.0 build
> tsc -b && vite build

vite v8.3.5 building client environment for production...
transforming...
✓ 2523 modules transformed.
rendering chunks...
computing gzip size...
dist/index.html                   1.31 kB │ gzip:   0.71 kB
dist/assets/index-BIY6GsLz.css   53.86 kB │ gzip:   9.34 kB
dist/assets/index-CCkN10ue.js   801.52 kB │ gzip: 219.32 kB
✓ built in 906ms
```
- **TypeScript Compiler (`tsc -b`)**: 0 type errors, 0 unused local warnings.
- **Production Asset Bundling**: Clean production assets generated in `dist/`.

---

## 8. Windows Installation & Startup Instructions

### Prerequisites
- MinGW-w64 GCC (GCC 6.3.0 or higher in system PATH)
- Node.js (v18 or higher) and npm

### Terminal 1: Backend Server
```powershell
cd "d:\OS CP\backend"

# Compile backend
mingw32-make all

# Run unit tests
mingw32-make test

# Start REST API server on port 8080
.\bin\os_platform_backend.exe --port 8080
```

### Terminal 2: Frontend Client
```powershell
cd "d:\OS CP\frontend"

# Install dependencies (first time only)
npm install

# Start Vite dev server on port 5173
npm run dev
```
Open **`http://localhost:5173/`** in any web browser.

---

## 9. Remaining Limitations & Operating System Differences

1. **Windows vs Linux Kernel Telemetry**:
   - On Windows, process CPU and memory metrics are sampled via Win32 Toolhelp32 snapshots and `GetProcessMemoryInfo`. Memory counters represent Working Set Size (WSS) and Commit Charge.
   - On Linux, metrics are parsed directly from `/proc/stat`, `/proc/meminfo`, and `/proc/[pid]/stat`. Resident Set Size (RSS) and Virtual Memory Size (VSZ) are derived from page table counters.
   - Operating system-specific metrics (such as page cache, dirty pages, and buffers) are accurately populated on Linux `/proc` but report unavailable or zero on Windows where equivalent kernel counters require privileged ETW/WMI drivers.
2. **Statistical Forecasting Scope**:
   - The memory forecasting module uses Ordinary Least Squares (OLS) linear regression. While mathematically exact on historical samples, linear extrapolation does not account for sudden application terminations or unexpected memory spikes.
3. **Advisory Scope**:
   - By design, the advisory engine does not execute automatic process termination (`kill -9`) or kernel configuration modifications (`sysctl -w`). All actions require human operator intervention.

---

## 10. Ubuntu / Linux Validation Instructions

To validate the platform on Ubuntu (e.g. Ubuntu 22.04 LTS / 24.04 LTS):

```bash
# 1. Install prerequisites
sudo apt-get update
sudo apt-get install -y build-essential gcc make nodejs npm

# 2. Build C backend
cd backend
make clean
make all

# 3. Execute native unit tests
make test
# Expected: 142 / 142 tests passing

# 4. Start backend
./bin/os_platform_backend --port 8080 &

# 5. Build and launch frontend
cd ../frontend
npm install
npm run build
npm run preview -- --port 5173
```

---

## 11. Project Demonstration Guide

Follow this step-by-step demonstration plan during project review:

### Scenario 1: Overview Dashboard & Live OS Telemetry
1. Open `http://localhost:5173/` on the **Overview** tab.
2. Observe live CPU utilization, available RAM, and swap counters updating in real-time with `Live OS Measurements` badges.
3. Review the **Subsystem Health & Pressure** bar showing active pressure categories and top priority recommendations.
4. Inspect the four cross-module analytical preview cards (CPU Scheduling preview, Memory regression slope, Page replacement benchmark, and Deadlock safety status).
5. Click **"Open Scheduling Module"** on the CPU Scheduling preview card to demonstrate cross-tab navigation.

### Scenario 2: Process Monitoring & Deep PID Inspection
1. Navigate to **Processes** from the sidebar.
2. In the search box, type a known process (e.g., `node`, `chrome`, or `svchost`).
3. Sort by **CPU %** or **Memory (MB)** descending to identify top resource consumers.
4. Click on any PID row to open the **Process Detail Modal**, displaying full command line, memory usage, state, and runtime metrics.

### Scenario 3: CPU Scheduling Algorithm Comparison
1. Navigate to **CPU Scheduling**.
2. Select the **"Mixed Workload"** preset (4 processes with varied burst and arrival times).
3. Click **"Run Scheduling Simulation"**.
4. Review the algorithm comparison table showing Average Waiting Time, Turnaround Time, and Response Time across all 5 algorithms (FCFS, SJF, SRTF, Priority, Round Robin).
5. Review the **Explainable Recommendation** panel explaining why SRTF or SJF is recommended for this workload.
6. Review the interactive Gantt chart showing time-sliced execution segments.

### Scenario 4: Memory Usage Prediction & OLS Regression
1. Navigate to **Memory Analytics**.
2. Observe the historical memory utilization curve.
3. Select a forecast horizon of **60 seconds** and click **"Generate Forecast"**.
4. Observe the linear regression trendline ($y = mx + c$), estimated time to $85\%$ and $95\%$ thresholds, and the transparent statistical disclaimer explaining model assumptions.

### Scenario 5: Demonstrating Belady's Anomaly in Page Replacement
1. Navigate to **Page Replacement**.
2. Click **"Belady's Anomaly Example"** button.
3. The reference string is loaded: `1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5`.
4. Observe the comparison between Frame Count = 3 and Frame Count = 4:
   - For FIFO: Faults increase from 9 (at 3 frames) to 10 (at 4 frames).
   - For Optimal: Faults decrease from 7 (at 3 frames) to 6 (at 4 frames).
5. Proves that increasing allocated physical frames can counter-intuitively increase page faults under FIFO.

### Scenario 6: Deadlock Avoidance (Banker's Algorithm) & Cycle Detection
1. Navigate to **Deadlock Analysis**.
2. Load the **"Silberschatz Textbook Safe State"** preset.
3. Click **"Run Banker's Safety Analysis"**.
4. Observe the verified safe sequence: `<P1 -> P3 -> P0 -> P2 -> P4>` with intermediate Work vector expansion.
5. In the Resource Request panel, test process $P_1$ requesting `[1, 0, 2]`. Verify that the request is granted safely.
6. Now load the **"Active Deadlock Cycle"** preset and click **"Detect Deadlock Cycles"**.
7. Observe the immediate identification of deadlocked processes involved in circular wait.

### Scenario 7: OS Log Analysis & Security Path Defense
1. Navigate to **Log Analysis**.
2. Click **"Load Linux Syslog Sample"** and click **"Analyze Logs"**.
3. Observe automated categorization into Memory/OOM, Storage/Disk, and Authentication categories.
4. Review recurring pattern extraction (e.g. Out-of-Memory killer events).
5. Switch to file analysis mode and analyze `backend/tests/sample.log` to demonstrate secure workspace file analysis.
6. Enter an invalid traversal path (e.g. `../../Windows/System32/config/SAM`) and verify the backend securely blocks access with HTTP `403 Forbidden`.

### Scenario 8: Advisory Resource Allocation Engine
1. Navigate to **Resource Advisory**.
2. Review synthesized recommendations generated across CPU, Memory, and Page Replacement subsystems.
3. Inspect the measured values vs defined threshold criteria and confidence percentages.
4. Review the platform's non-destructive safety policy confirming no automatic process termination or kernel mutations take place.
