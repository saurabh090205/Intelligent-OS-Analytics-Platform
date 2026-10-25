# Intelligent Operating System Analytics and Resource Management Platform

An enterprise-grade operating system telemetry, process monitoring, scheduling simulation, and resource management platform developed in C with a modern React, Vite, and TypeScript frontend.

---

## 1. Project Overview

The **Intelligent OS Analytics and Resource Management Platform** is a full-stack systems software project that interfaces directly with low-level kernel interfaces (such as the Linux `/proc` virtual filesystem and Windows Win32 APIs) to deliver live telemetry, process lifecycle monitoring, algorithmic scheduling recommendation, memory forecasting, page replacement simulation, deadlock detection & avoidance, OS log analysis, and an advisory resource allocation engine.

### Core Objectives
1. **Low-overhead Real-time Telemetry**: Native C engine sampling CPU, memory, and process counters with minimal CPU and memory footprint.
2. **Dual-Environment Virtual Filesystem / Host APIs**: Linux `/proc` filesystem (`/proc/stat`, `/proc/meminfo`, `/proc/[pid]/stat`, `/proc/[pid]/cmdline`, `/proc/uptime`) with Win32/Winsock fallback for cross-platform execution.
3. **Consecutive Differential Sampling**: Calculate real process and system CPU utilization from consecutive jiffy delta samples.
4. **Algorithmic Simulation & Modeling**: High-fidelity implementations of classic and modern operating system algorithms (CPU Scheduling, Linear Regression Memory Forecasting, Page Replacement, Dijkstra's Banker's Algorithm, Circular Deadlock Detection).
5. **Safe Log Telemetry & Pattern Analysis**: Multi-format log stream analysis with path-traversal security defenses rejecting unauthorized access to sensitive files.
6. **Advisory Resource Allocation Engine**: Non-destructive analytical guidance synthesizing telemetry and algorithmic benchmarks without automatic process termination.
7. **Unified Executive Dashboard**: Integrated high-density React frontend visualizing live measurements, subsystem pressure indicators, and cross-module analytical previews.

---

## 2. Technology Stack

### Backend
- **Language**: C (C99 / GNU99 standard)
- **Compiler**: GCC (MinGW-w64 on Windows, GNU GCC on Linux)
- **System Interfaces**: Linux `/proc` virtual filesystem, Win32 Toolhelp32 & SystemInfo APIs, POSIX sockets & Winsock2
- **Cross-Platform Support**: Conditional compilation (`#if defined(_WIN32)`) for native Windows execution and future Ubuntu/Linux validation
- **JSON Serialization**: Custom zero-copy dynamic buffer builder (`src/utils/json_utils.c`)
- **HTTP Engine**: Portable non-blocking socket HTTP/1.1 REST server with CORS headers, keep-alive termination, and socket timeouts
- **Build System**: GNU Make (`Makefile`)

### Frontend
- **Framework**: React 19 + TypeScript
- **Tooling**: Vite 8 with `@tailwindcss/vite`
- **Styling**: Tailwind CSS v4 with Dark Navy / Emerald / Cyan palette
- **Visualization**: Recharts (Streaming AreaChart, Donut PieChart, BarChart)
- **Icons**: Lucide React
- **Architecture**: Modular component hierarchy with type-safe REST API client

---

## 3. Project Directory Structure

```
intelligent-os-platform/
├── backend/
│   ├── src/
│   │   ├── main.c                  # Server entry point & signal handling
│   │   ├── api/
│   │   │   ├── server.c            # Socket HTTP server, timeouts & CORS handling
│   │   │   └── routes.c            # REST API routing & response handlers
│   │   ├── monitoring/
│   │   │   ├── process_monitor.c   # /proc scanner, Toolhelp32 snapshot, CPU sampling
│   │   │   ├── cpu_monitor.c       # /proc/stat CPU jiffy differential calculator
│   │   │   └── memory_monitor.c    # /proc/meminfo RAM & swap utilization parser
│   │   ├── scheduling/
│   │   │   └── cpu_scheduler.c     # FCFS, SJF, SRTF, Priority, Round Robin engines
│   │   ├── memory/
│   │   │   ├── memory_prediction.c # Moving average & OLS linear regression forecast
│   │   │   └── page_replacement.c  # FIFO, LRU, Optimal page replacement simulators
│   │   ├── deadlock/
│   │   │   └── deadlock.c          # Banker's algorithm, safety verification, cycle detector
│   │   ├── logs/
│   │   │   └── log_analyzer.c      # Multi-format syslog parser & security path validator
│   │   ├── advisory/
│   │   │   └── resource_advisory.c # Advisory guidance engine & pressure synthesizer
│   │   └── utils/
│   │       └── json_utils.c        # Dynamic JSON serializer with string escaping
│   ├── include/
│   │   ├── process_monitor.h       # Process table structures & prototypes
│   │   ├── cpu_monitor.h           # CPU core and jiffy structures
│   │   ├── memory_monitor.h        # Memory counters & utilization prototypes
│   │   ├── cpu_scheduler.h         # Scheduling algorithms & workload types
│   │   ├── memory_prediction.h     # History ring buffer & forecast structures
│   │   ├── page_replacement.h     # Page replacement simulation prototypes
│   │   ├── deadlock.h              # Banker's algorithm & matrix structures
│   │   ├── log_analyzer.h          # Log parsing, categorization & path validation
│   │   ├── resource_advisory.h     # Advisory engine structures & thresholds
│   │   ├── server.h                # Server configuration & lifecycle
│   │   ├── routes.h                # Route dispatcher & response wrappers
│   │   └── json_utils.h            # JSON builder declarations
│   ├── tests/
│   │   ├── test_monitoring.c       # Phase 1 unit test suite (27 test cases)
│   │   ├── test_phase2.c           # Phase 2 unit test suite (53 test cases)
│   │   ├── test_phase3.c           # Phase 3 & 4 unit test suite (62 test cases)
│   │   └── sample.log              # Verified test log stream for path analysis
│   └── Makefile                    # Backend compilation & test targets
├── frontend/
│   ├── src/
│   │   ├── components/
│   │   │   ├── MetricCard.tsx      # High-density telemetry metric card
│   │   │   ├── Sidebar.tsx         # Dark navy navigation drawer with all 7 modules
│   │   │   ├── TopNavbar.tsx       # Live status, refresh controls & interval picker
│   │   │   └── ProcessModal.tsx    # PID deep inspection modal
│   │   ├── pages/
│   │   │   ├── OverviewPage.tsx    # Integrated composite dashboard
│   │   │   ├── ProcessMonitorPage.tsx # Live process table with sort & filter
│   │   │   ├── CpuSchedulingPage.tsx  # Interactive scheduling simulator & Gantt charts
│   │   │   ├── MemoryAnalyticsPage.tsx# Time-series memory forecasting & OLS regression
│   │   │   ├── PageReplacementPage.tsx# FIFO, LRU, Optimal comparator & Belady anomaly
│   │   │   ├── DeadlockPage.tsx       # Banker's safety checker, request test & cycle detector
│   │   │   ├── LogAnalysisPage.tsx    # Multi-format log parser, burst detection & filters
│   │   │   ├── ResourceAdvisoryPage.tsx # Synthesized recommendations & safety policy
│   │   │   └── SettingsAboutPage.tsx  # Architecture reference & live API test console
│   │   ├── services/
│   │   │   └── api.ts              # Type-safe REST client for all 16 endpoints
│   │   ├── types/
│   │   │   └── system.ts           # TypeScript interfaces for all system models
│   │   ├── App.tsx                 # Core shell, tab navigation & state coordinator
│   │   └── index.css               # Design system tokens and styling
│   ├── package.json
│   └── vite.config.ts              # Vite configuration with proxy to backend
├── scripts/
│   ├── e2e_api_test.js             # Automated 29-test end-to-end API integration suite
│   ├── start_backend.bat           # Windows backend launch script
│   ├── start_frontend.bat          # Windows frontend launch script
│   ├── start_backend.sh            # Linux backend launch script
│   └── start_frontend.sh           # Linux frontend launch script
├── ARCHITECTURE.md                 # System architecture documentation
├── API_DOCUMENTATION.md            # REST API reference, request/response schemas & security
├── TESTING.md                      # Complete unit test, integration test & security reports
├── PROJECT_REPORT.md               # Final Phase 4 comprehensive project report
└── README.md                       # This document
```

---

## 4. Module Descriptions & Telemetry Clarification

The platform strictly distinguishes between **Live Host Telemetry**, **Algorithmic Simulations**, and **Statistical Forecasts**:

| Module | Classification | Description & Methodology |
| :--- | :--- | :--- |
| **1. Process & Hardware Monitoring** | **Live Telemetry** | Samples actual operating system state using Win32 API snapshot (`CreateToolhelp32Snapshot`) on Windows and `/proc` virtual filesystem on Linux. Computes consecutive differential jiffy deltas for true CPU utilization. |
| **2. CPU Scheduling** | **Algorithmic Simulation** | Simulates FCFS, SJF, SRTF, Priority, and Round Robin on discrete workload vectors. Computes turnaround time, waiting time, response time, and context switches. Recommends the optimal algorithm with explainable rationale. |
| **3. Memory Usage Prediction** | **Statistical Forecast** | Takes recorded physical memory utilization history samples and applies Ordinary Least Squares (OLS) linear regression ($y = mx + c$) and simple moving averages to project future exhaustion thresholds ($85\%$ and $95\%$). Honest disclaimer clarifies statistical assumptions. |
| **4. Page Replacement** | **Algorithmic Simulation** | Simulates FIFO, LRU, and Optimal algorithms on arbitrary reference strings with configurable frame counts (1 to 16). Proves theoretical lower bounds and demonstrates Belady's Anomaly where FIFO page faults increase as frames increase. |
| **5. Deadlock Analysis** | **Algorithmic Modeling** | Implements Dijkstra's Banker's Algorithm for safety verification and safe sequence generation. Simulates dynamic resource allocation requests and executes cycle detection on Resource Allocation Graphs. Distinguishes Unsafe States from Confirmed Deadlocks. |
| **6. Operating System Log Analysis** | **Stream Analysis & Parsing** | Parses Linux syslog, ISO 8601, and Windows event log formats. Identifies recurring error patterns, rapid error bursts, and categorizes events (OOM, Disk, Auth, Crash). Safe path validator protects against path traversal attacks. |
| **7. Advisory Resource Allocation** | **Advisory Engine** | Synthesizes live host measurements (CPU, RAM, swap) with analytical models (memory forecast slope, page replacement hit rates) to generate prioritized recommendations with confidence scores. Operates under a non-destructive safety policy (no automated process termination). |

---

## 5. Quick Start Instructions (Windows Environment)

### Prerequisites
- **MinGW-w64 GCC**: Installed and available in PATH (GCC 6.3.0+).
- **Node.js & npm**: Node.js v18+ (tested with Node.js v24.13.0).

### Step 1: Compile Backend & Run Unit Tests
```powershell
cd "d:\OS CP\backend"

# Compile all modules and the backend server binary
mingw32-make all

# Run all 142 automated unit tests
mingw32-make test
```

### Step 2: Start the C Backend Server
```powershell
cd "d:\OS CP"

# Start HTTP server on port 8080
.\backend\bin\os_platform_backend.exe --port 8080
```
The server will output:
```
====================================================
 Intelligent OS Analytics and Resource Manager
 HTTP REST Server listening on http://127.0.0.1:8080
 Ready for frontend connections and API requests.
====================================================
```

### Step 3: Start the Frontend Application
In a separate terminal window:
```powershell
cd "d:\OS CP\frontend"

# Install dependencies (if first time)
npm install

# Start Vite development server
npm run dev
```
Open your browser to: **`http://localhost:5173/`**

### Step 4: Run the Automated Integration Test Suite
In a separate terminal window with the backend running:
```powershell
cd "d:\OS CP"
node scripts/e2e_api_test.js
```
Expected output: **29 / 29 Passed (100.0%)**.

### Step 5: Build Frontend for Production
```powershell
cd "d:\OS CP\frontend"
npm run build
```
Expected output: Built successfully in < 1 second with 0 errors.

---

## 6. Ubuntu / Linux Validation Instructions (For Future Execution)

The backend C codebase has been written with dual-target portability using POSIX APIs and conditional compilation `#if defined(_WIN32) ... #else ...`. To validate on Ubuntu:

```bash
# 1. Update and install build prerequisites
sudo apt-get update
sudo apt-get install -y build-essential gcc make nodejs npm

# 2. Compile backend using Linux GCC
cd backend
make clean
make all

# 3. Execute unit tests natively on Linux
make test

# 4. Start backend on Linux (will read directly from /proc/stat and /proc/meminfo)
./bin/os_platform_backend --port 8080 &

# 5. Build and launch frontend
cd ../frontend
npm install
npm run build
npm run preview -- --port 5173
```

---

## 7. Security & Safety Boundaries

1. **Path Traversal Defense**: The log analyzer rejects any file path containing directory traversal sequences (`..`), system hives (`system32`, `config/sam`, `/etc/shadow`), or executable binaries (`.exe`, `.dll`, `.sys`, `.bin`), returning HTTP `403 Forbidden`.
2. **File Size Limits**: File analysis rejects targets exceeding 5 MB with HTTP `413 Payload Too Large`.
3. **PID Validation**: Process inspection strictly validates PID parameters, rejecting negative or non-numeric arguments with HTTP `400 Bad Request` and non-existent PIDs with HTTP `404 Not Found`.
4. **Socket Timeout Protection**: All client socket connections enforce 4-second receive and send timeouts (`SO_RCVTIMEO` / `SO_SNDTIMEO`) to protect against slowloris attacks and connection hangs.
5. **Non-Destructive Advisory Policy**: The platform is strictly advisory. It intentionally omits automated process termination, nice level mutation, or kernel parameter reconfiguration to prevent accidental system instability.

---

## 8. License & Academic Attribution
Developed as an advanced Operating Systems course project demonstrating systems programming in C, kernel interface instrumentation, concurrency, and algorithm design.
