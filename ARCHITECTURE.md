# System Architecture Document

## Intelligent OS Analytics and Resource Management Platform

This document describes the architectural design, component separation, communication protocols, and data models of the platform.

---

## 1. High-Level System Architecture

```mermaid
graph TD
    subgraph LinuxKernel ["Linux Operating System / Kernel"]
        ProcStat["/proc/stat<br/>(CPU jiffies & cores)"]
        ProcMem["/proc/meminfo<br/>(RAM, swap, cached)"]
        ProcPid["/proc/[pid]/stat<br/>(Process ticks, RSS, VSZ)"]
        ProcCmd["/proc/[pid]/cmdline<br/>(Process invocation)"]
        ProcUp["/proc/uptime<br/>(System uptime)"]
    end

    subgraph CBackend ["C Backend Engine (Port 8080)"]
        subgraph MonitoringLayer ["Monitoring Subsystems"]
            CPUMon["cpu_monitor.c<br/>(Delta Jiffy Calculator)"]
            MemMon["memory_monitor.c<br/>(MemAvailable Parser)"]
            ProcMon["process_monitor.c<br/>(Process Scanner & Cache)"]
        end

        subgraph APILayer ["REST API Layer"]
            Router["routes.c<br/>(Route Dispatcher)"]
            HTTPServer["server.c<br/>(Portable Socket Engine)"]
            JSONUtil["json_utils.c<br/>(Zero-copy Buffer Builder)"]
        end
    end

    subgraph ReactFrontend ["React Frontend (Port 5173)"]
        ApiClient["services/api.ts<br/>(Type-safe Fetch Client)"]
        StateEngine["App.tsx<br/>(Polling Loop & State Engine)"]
        
        subgraph UIComponents ["UI Component Architecture"]
            Nav["Sidebar.tsx & TopNavbar.tsx"]
            Overview["OverviewPage.tsx<br/>(6 Metrics + Recharts)"]
            ProcView["ProcessMonitorPage.tsx<br/>(Sortable Table + Filter)"]
            Modal["ProcessModal.tsx<br/>(PID Deep Inspector)"]
            Upcoming["ModulePlaceholderPage.tsx<br/>(Phases 2 & 3 Previews)"]
            Settings["SettingsAboutPage.tsx<br/>(Live API Diagnostics)"]
        end
    end

    ProcStat --> CPUMon
    ProcMem --> MemMon
    ProcPid --> ProcMon
    ProcCmd --> ProcMon
    ProcUp --> ProcMon

    CPUMon --> Router
    MemMon --> Router
    ProcMon --> Router

    Router --> JSONUtil
    JSONUtil --> HTTPServer
    HTTPServer -->|HTTP/1.1 JSON (Port 8080)| ApiClient

    ApiClient --> StateEngine
    StateEngine --> UIComponents
```

---

## 2. Backend Architecture (C Engine)

The backend is built from modular, single-responsibility C source files adhering to standard C99/GNU99.

### 2.1 CPU Monitoring Engine (`backend/src/monitoring/cpu_monitor.c`)
- **Source**: `/proc/stat`
- **Algorithm**:
  $$\text{Total Jiffies} = \text{user} + \text{nice} + \text{system} + \text{idle} + \text{iowait} + \text{irq} + \text{softirq} + \text{steal}$$
  $$\text{Idle Jiffies} = \text{idle} + \text{iowait}$$
  $$\Delta_{\text{total}} = \text{Total}_{\text{current}} - \text{Total}_{\text{previous}}$$
  $$\Delta_{\text{idle}} = \text{Idle}_{\text{current}} - \text{Idle}_{\text{previous}}$$
  $$\text{CPU Utilization} = \left(1.0 - \frac{\Delta_{\text{idle}}}{\Delta_{\text{total}}}\right) \times 100\%$$
- **Consecutive Sampling**: Metrics are computed dynamically from the difference between the current read and the previous sample.
- **Clamping**: Bounded strictly within $[0.0\%, 100.0\%]$.

### 2.2 Memory Monitoring Engine (`backend/src/monitoring/memory_monitor.c`)
- **Source**: `/proc/meminfo`
- **Fields Extracted**: `MemTotal`, `MemFree`, `MemAvailable`, `Buffers`, `Cached`, `SwapTotal`, `SwapFree`.
- **Utilization Calculation**:
  $$\text{Used Memory} = \text{MemTotal} - \text{MemAvailable}$$
  $$\text{Memory Utilization} = \frac{\text{Used Memory}}{\text{MemTotal}} \times 100\%$$
- Handles older kernels ($\le 3.14$) where `MemAvailable` is absent by falling back to $\text{MemFree} + \text{Buffers} + \text{Cached}$.

### 2.3 Process Monitoring Engine (`backend/src/monitoring/process_monitor.c`)
- **Directory Traversal**: Uses POSIX `opendir("/proc")` and filters directory names consisting solely of digits.
- **Robust Comm Parsing**: Because Linux process names in `/proc/[pid]/stat` can contain spaces and parentheses (e.g., `(Web Content)`), the parser anchors on the first `(` and the *last* `)` to extract the command name cleanly.
- **State Character Translation**:
  - `R`: Running
  - `S`: Sleeping (interruptible wait)
  - `D`: Disk Sleep (uninterruptible wait)
  - `Z`: Zombie (terminated, awaiting waitpid)
  - `T`/`t`: Stopped / Tracing stop
  - `I`: Idle kernel thread
- **Process CPU Calculation**:
  Stores a dynamic tracking cache of `(pid, prev_ticks, prev_timestamp)`.
  $$\Delta_{\text{proc\_ticks}} = (\text{utime} + \text{stime})_{\text{current}} - (\text{utime} + \text{stime})_{\text{previous}}$$
  $$\text{Process CPU } \% = \frac{\Delta_{\text{proc\_ticks}} / \text{CLK\_TCK}}{\Delta_{\text{timestamp}}} \times 100\%$$
- **Handling Vanishing and Inaccessible Processes**:
  If `fopen` fails due to `ENOENT` (process died between `readdir` and `open`) or `EACCES` (permission denied), the scanner skips the entry safely without crashing or leaking file descriptors.

### 2.4 Dynamic JSON Builder (`backend/src/utils/json_utils.c`)
- Self-contained, dynamic memory buffer serializer.
- Implements full string escaping for quotes, backslashes, tabs, carriage returns, and control characters ($\le 0x1F$).
- Prevents buffer overflows via automatic geometric capacity expansion (`realloc`).

### 2.5 REST HTTP Server (`backend/src/api/server.c`)
- Portable socket networking supporting both Linux POSIX sockets and Win32 Winsock.
- Binds to localhost (`127.0.0.1`) on configurable port (default: 8080).
- Sets `SO_REUSEADDR` to allow rapid restart without socket binding delays.
- Generates standard HTTP/1.1 headers:
  - `Content-Type: application/json; charset=utf-8`
  - `Access-Control-Allow-Origin: *`
  - `Access-Control-Allow-Methods: GET, POST, OPTIONS`
  - `Access-Control-Allow-Headers: Content-Type, Authorization`
  - `Connection: close`

---

## 3. Frontend Architecture (React + Vite + TypeScript)

### 3.1 Component Hierarchy
- **`App.tsx`**: Central orchestrator managing global state, polling timer, connection health, and active route view.
  - **`Sidebar.tsx`**: Left fixed navigation bar with brand badge, module routes with phase indicators, and backend connectivity pulse.
  - **`TopNavbar.tsx`**: Header bar with breadcrumbs, live search input, uptime counter, refresh rate dropdown (1s, 2s, 5s, 10s, Pause), manual refresh button, and alert drawer.
  - **`OverviewPage.tsx`**:
    - 6 x `MetricCard.tsx`: CPU %, Memory %, Available RAM, Total Processes, Running Tasks, Uptime.
    - 2 x Recharts `AreaChart`: Real-time streaming time series for CPU and Memory.
    - Recharts `PieChart`: Donut chart showing process state distribution.
    - Top 5 CPU and Memory consumers list with direct click-to-inspect action.
  - **`ProcessMonitorPage.tsx`**:
    - Filter pills (All, Running, Sleeping, Stopped, Zombie).
    - Sortable table columns with directional arrows (PID, Name, State, CPU %, Memory RSS, % RAM, Threads, PPID).
    - Pagination controls and row limit selector.
  - **`ProcessModal.tsx`**: Modal drawer displaying detailed telemetry for a single process.
  - **`ModulePlaceholderPage.tsx`**: Wireframes, theoretical concepts, and planned endpoints for Modules 2–8.
  - **`SettingsAboutPage.tsx`**: Architecture specifications and live API diagnostics console.

---

## 4. Security & Privilege Model

1. **Non-Root Safe Execution**: The C backend exclusively reads user-accessible virtual files under `/proc`. No kernel modifications, process termination, or priority mutations are performed.
2. **Localhost Binding**: Sockets bind to `127.0.0.1` by default to prevent unauthorized external network exposure.
3. **No Shell Injections**: No external shell commands (`system()`, `popen()`) are constructed from user input.
4. **Input Sanitization**: PID path parameters are strictly validated to contain only positive numeric digits before routing.
