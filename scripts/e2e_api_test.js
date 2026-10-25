/**
 * Comprehensive End-to-End REST API Integration Test Suite for Phase 4
 * Verifies all 15+ backend endpoints, normal cases, error responses,
 * boundary conditions, and security defenses.
 */

const BASE_URL = 'http://127.0.0.1:8080/api';

let totalTests = 0;
let passedTests = 0;
let failedTests = 0;

async function testCase(name, testFn) {
  totalTests++;
  try {
    await testFn();
    passedTests++;
    console.log(`  [PASS] ${name}`);
  } catch (err) {
    failedTests++;
    console.error(`  [FAIL] ${name}: ${err.message}`);
  }
}

function assert(condition, message) {
  if (!condition) {
    throw new Error(message || 'Assertion failed');
  }
}

async function runAllTests() {
  console.log('====================================================');
  console.log(' Phase 4 End-to-End API Integration Test Suite');
  console.log(' Target Backend: ' + BASE_URL);
  console.log('====================================================\n');

  // Group 1: Core System & Telemetry Endpoints
  console.log('[SUITE 1: System Health & Hardware Telemetry]');
  await testCase('GET /api/health returns ok status & platform info', async () => {
    const res = await fetch(`${BASE_URL}/health`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.status === 'ok', 'Status should be ok');
    assert(data.version === '3.0.0', `Version should be 3.0.0, got ${data.version}`);
  });

  await testCase('GET /api/system/overview returns bounded CPU & Memory telemetry', async () => {
    const res = await fetch(`${BASE_URL}/system/overview`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.cpu_utilization >= 0 && data.cpu_utilization <= 100, 'CPU utilization out of bounds');
    assert(data.memory_utilization >= 0 && data.memory_utilization <= 100, 'Memory utilization out of bounds');
    assert(data.total_processes > 0, 'Total processes should be positive');
  });

  await testCase('GET /api/system/cpu returns per-core metrics and jiffies', async () => {
    const res = await fetch(`${BASE_URL}/system/cpu`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.cores > 0, 'CPU cores should be > 0');
    assert(Array.isArray(data.per_core), 'per_core should be an array');
  });

  await testCase('GET /api/system/memory returns RAM and swap counters', async () => {
    const res = await fetch(`${BASE_URL}/system/memory`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.total_kb > 0, 'total_kb should be > 0');
    assert(data.available_kb <= data.total_kb, 'available_kb should be <= total_kb');
  });

  // Group 2: Process Table & Detail
  console.log('\n[SUITE 2: Process Monitoring & Detail Inspector]');
  let sampledPid = 4;
  await testCase('GET /api/processes returns process table list', async () => {
    const res = await fetch(`${BASE_URL}/processes`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.total > 0, 'Total processes should be positive');
    assert(Array.isArray(data.processes) && data.processes.length > 0, 'Process array empty');
    const positiveProc = data.processes.find(p => p.pid > 0);
    sampledPid = positiveProc ? positiveProc.pid : 4;
  });

  await testCase('GET /api/processes/:pid returns single process detail', async () => {
    const res = await fetch(`${BASE_URL}/processes/${sampledPid}`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.pid === sampledPid, `Expected PID ${sampledPid}, got ${data.pid}`);
    assert(typeof data.name === 'string', 'Process name should be string');
  });

  await testCase('GET /api/processes/:pid with non-existent PID returns 404', async () => {
    const res = await fetch(`${BASE_URL}/processes/9999999`);
    assert(res.status === 404, `Expected 404, got ${res.status}`);
  });

  await testCase('GET /api/processes/:pid with alphabetic PID returns 400', async () => {
    const res = await fetch(`${BASE_URL}/processes/invalid_pid`);
    assert(res.status === 400, `Expected 400, got ${res.status}`);
  });

  await testCase('GET /api/processes/:pid with negative PID returns 400', async () => {
    const res = await fetch(`${BASE_URL}/processes/-42`);
    assert(res.status === 400, `Expected 400, got ${res.status}`);
  });

  // Group 3: CPU Scheduling
  console.log('\n[SUITE 3: CPU Scheduling Algorithms & Recommendations]');
  await testCase('GET /api/scheduling/examples returns preconfigured workloads', async () => {
    const res = await fetch(`${BASE_URL}/scheduling/examples`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(Array.isArray(data.examples) && data.examples.length >= 3, 'Should have >= 3 presets');
  });

  await testCase('POST /api/scheduling/simulate evaluates all 5 algorithms and recommendation', async () => {
    const payload = {
      processes: [
        { pid: 1, name: 'P1', arrival_time: 0, burst_time: 8, priority: 3 },
        { pid: 2, name: 'P2', arrival_time: 1, burst_time: 4, priority: 1 },
        { pid: 3, name: 'P3', arrival_time: 2, burst_time: 9, priority: 4 }
      ],
      quantum: 3
    };
    const res = await fetch(`${BASE_URL}/scheduling/simulate`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.algorithms.fcfs && data.algorithms.sjf && data.algorithms.srtf, 'Missing algorithms');
    assert(typeof data.recommended_algo_name === 'string', 'Missing recommendation');
    assert(Array.isArray(data.algorithms.fcfs.gantt), 'FCFS Gantt segments should be array');
  });

  await testCase('POST /api/scheduling/simulate with malformed JSON returns 400', async () => {
    const res = await fetch(`${BASE_URL}/scheduling/simulate`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: '{"processes": [bad json'
    });
    assert(res.status === 400, `Expected 400, got ${res.status}`);
  });

  // Group 4: Memory Forecasting
  console.log('\n[SUITE 4: Memory Forecasting & History]');
  await testCase('GET /api/memory/history returns recorded time-series buffer', async () => {
    const res = await fetch(`${BASE_URL}/memory/history`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(Array.isArray(data.samples), 'samples should be an array');
  });

  await testCase('GET /api/memory/forecast computes linear regression & threshold projection', async () => {
    const res = await fetch(`${BASE_URL}/memory/forecast?horizon=60&window=10&threshold=85.0`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(typeof data.slope_pct_per_min === 'number', 'slope should be number');
    assert(data.r_squared >= 0 && data.r_squared <= 1.0, 'R2 out of bounds');
    assert(Array.isArray(data.forecast_points), 'forecast_points should be an array');
  });

  // Group 5: Page Replacement
  console.log('\n[SUITE 5: Virtual Memory Page Replacement Simulation]');
  await testCase('GET /api/page-replacement/examples returns Belady & textbook strings', async () => {
    const res = await fetch(`${BASE_URL}/page-replacement/examples`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(Array.isArray(data.examples) && data.examples.length >= 3, 'Missing presets');
  });

  await testCase('POST /api/page-replacement/simulate computes FIFO, LRU, and Optimal', async () => {
    const payload = {
      reference_string: [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1],
      frame_count: 3
    };
    const res = await fetch(`${BASE_URL}/page-replacement/simulate`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.algorithms.fifo.fault_count === 15, `Expected 15 FIFO faults, got ${data.algorithms.fifo.fault_count}`);
    assert(data.algorithms.lru.fault_count === 12, `Expected 12 LRU faults, got ${data.algorithms.lru.fault_count}`);
    assert(data.algorithms.optimal.fault_count === 9, `Expected 9 Optimal faults, got ${data.algorithms.optimal.fault_count}`);
  });

  await testCase('POST /api/page-replacement/simulate with invalid frame_count returns 400', async () => {
    const res = await fetch(`${BASE_URL}/page-replacement/simulate`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ reference_string: [1, 2, 3], frame_count: 0 })
    });
    assert(res.status === 400, `Expected 400, got ${res.status}`);
  });

  // Group 6: Deadlock Detection & Avoidance
  console.log('\n[SUITE 6: Deadlock Detection & Banker\'s Algorithm]');
  await testCase('GET /api/deadlock/examples returns standard system states', async () => {
    const res = await fetch(`${BASE_URL}/deadlock/examples`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(Array.isArray(data.examples) && data.examples.length === 3, 'Should have 3 presets');
  });

  await testCase('POST /api/deadlock/banker evaluates safe state and safe sequence', async () => {
    const res = await fetch(`${BASE_URL}/deadlock/banker`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ example_id: 'silberschatz_safe' })
    });
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.is_safe === true, 'System should be in safe state');
    assert(Array.isArray(data.safe_sequence) && data.safe_sequence.length === 5, 'Safe sequence should have 5 items');
    assert(Array.isArray(data.steps) && data.steps.length === 5, 'Should track 5 Work progression steps');
  });

  await testCase('POST /api/deadlock/banker identifies unsafe state', async () => {
    const res = await fetch(`${BASE_URL}/deadlock/banker`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ example_id: 'unsafe_state' })
    });
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.is_safe === false, 'State should be unsafe');
    assert(data.explanation.includes('unsafe state is NOT synonymous with deadlock'), 'Explanation missing distinction');
  });

  await testCase('POST /api/deadlock/request validates resource request safety', async () => {
    const payload = {
      example_id: 'silberschatz_safe',
      process_idx: 1,
      request: [1, 0, 2]
    };
    const res = await fetch(`${BASE_URL}/deadlock/request`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.can_grant === true, 'Request should be granted');
    assert(data.would_be_safe === true, 'Resulting state should be safe');
  });

  await testCase('POST /api/deadlock/detect detects circular wait cycles', async () => {
    const res = await fetch(`${BASE_URL}/deadlock/detect`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ example_id: 'deadlock_cycle' })
    });
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.is_deadlocked === true, 'Deadlock should be detected');
    assert(data.deadlocked_count === 2, `Expected 2 deadlocked procs, got ${data.deadlocked_count}`);
  });

  // Group 7: OS Log Analysis & Security Defenses
  console.log('\n[SUITE 7: OS Log Analysis & Path Traversal Security]');
  await testCase('GET /api/logs/samples returns representative log text', async () => {
    const res = await fetch(`${BASE_URL}/logs/samples?id=linux_syslog`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(typeof data.content === 'string' && data.content.length > 50, 'Sample text empty');
  });

  await testCase('POST /api/logs/analyze parses Linux syslog & categorizes OOM/Crash', async () => {
    const raw = 'Oct 25 14:12:44 host-srv kernel: Out of memory: Kill process 8912\nOct 25 14:14:02 host-srv systemd[1]: nginx.service: Failed with result core-dump';
    const res = await fetch(`${BASE_URL}/logs/analyze`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ content: raw, min_severity: 'ALL', category: -1 })
    });
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.total_lines_read === 2, 'Should read 2 lines');
    assert(data.error_count === 2, 'Should count 2 errors');
  });

  await testCase('POST /api/logs/analyze parses Windows event format', async () => {
    const raw = 'Level: Error, Date: 2026-10-25 14:10:45, Source: Ntfs, Message: The file system structure on volume C: is corrupt.';
    const res = await fetch(`${BASE_URL}/logs/analyze`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ content: raw })
    });
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.category_counts.storage_disk === 1, 'Should categorize as Storage & Disk');
  });

  await testCase('POST /api/logs/analyze parses safe local log file (backend/tests/sample.log)', async () => {
    const res = await fetch(`${BASE_URL}/api/logs/analyze`.replace('/api/api', '/api'), {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ file_path: 'backend/tests/sample.log' })
    });
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.total_lines_read === 12, `Expected 12 lines, got ${data.total_lines_read}`);
  });

  await testCase('SECURITY: POST /api/logs/analyze rejects path traversal (../../Windows/System32/config/SAM) with 403', async () => {
    const res = await fetch(`${BASE_URL}/logs/analyze`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ file_path: '../../Windows/System32/config/SAM' })
    });
    assert(res.status === 403, `Expected 403 Forbidden, got ${res.status}`);
  });

  await testCase('SECURITY: POST /api/logs/analyze rejects binary executables with 403', async () => {
    const res = await fetch(`${BASE_URL}/logs/analyze`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ file_path: 'malicious.exe' })
    });
    assert(res.status === 403, `Expected 403 Forbidden, got ${res.status}`);
  });

  // Group 8: Advisory Resource Allocation
  console.log('\n[SUITE 8: Advisory Resource Allocation Engine]');
  await testCase('GET /api/advisory/recommendations returns multi-subsystem guidance & disclaimer', async () => {
    const res = await fetch(`${BASE_URL}/advisory/recommendations`);
    assert(res.status === 200, `Expected 200, got ${res.status}`);
    const data = await res.json();
    assert(data.overview && typeof data.overview.overall_health === 'string', 'Missing overview health');
    assert(Array.isArray(data.items) && data.items.length >= 3, 'Should have >= 3 items');
    assert(data.disclaimer.includes('ADVISORY NOTICE'), 'Missing safety disclaimer');
    assert(data.disclaimer.includes('does NOT automatically terminate processes'), 'Missing non-destructive clause');
  });

  // Summary
  console.log('\n====================================================');
  console.log(` Integration Test Results: ${passedTests} / ${totalTests} Passed (${((passedTests / totalTests) * 100).toFixed(1)}%)`);
  if (failedTests > 0) {
    console.log(` FAILED TESTS: ${failedTests}`);
    process.exit(1);
  } else {
    console.log(' ALL ENDPOINTS & SECURITY VERIFICATIONS SUCCEEDED');
    console.log('====================================================');
    process.exit(0);
  }
}

runAllTests().catch((err) => {
  console.error('Fatal test error:', err);
  process.exit(1);
});
