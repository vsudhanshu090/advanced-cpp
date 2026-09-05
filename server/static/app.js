let editor = null;
let currentProblemId = null;
let allProblems = [];
let activeDifficulties = new Set(); // empty = show all

const DIFFICULTIES = ["easy", "easy-medium", "medium", "hard", "expert"];

async function loadHome() {
  const [problemsRes, statsRes] = await Promise.all([
    fetch("/api/problems"), fetch("/api/stats")
  ]);
  allProblems = await problemsRes.json();
  const stats = await statsRes.json();

  renderDiffChips();
  renderProblemList();
  renderStats(stats);
}

function renderDiffChips() {
  const container = document.getElementById("diff-chips");
  container.innerHTML = "";
  for (const d of DIFFICULTIES) {
    const chip = document.createElement("div");
    chip.className = "diff-chip" + (activeDifficulties.has(d) ? " active" : "");
    chip.dataset.diff = d;
    chip.textContent = d.replace("-", " ");
    chip.addEventListener("click", () => {
      if (activeDifficulties.has(d)) activeDifficulties.delete(d);
      else activeDifficulties.add(d);
      renderDiffChips();
      renderProblemList();
    });
    container.appendChild(chip);
  }
}

function renderProblemList() {
  const listEl = document.getElementById("problem-list");
  listEl.innerHTML = "";
  const filtered = activeDifficulties.size === 0
    ? allProblems
    : allProblems.filter(p => activeDifficulties.has(p.difficulty));

  for (const p of filtered) {
    const item = document.createElement("div");
    item.className = "problem-item";
    item.dataset.id = p.id;
    if (p.id === currentProblemId) item.classList.add("active");

    const topicsHtml = (p.topics || [])
      .map(t => `<span class="topic-tag">${t}</span>`).join("");

    item.innerHTML = `
      <div class="problem-item-top">
        <span><span class="diff-dot ${p.difficulty}"></span>${p.title}</span>
        ${p.solved ? '<span class="solved-check">&#10003;</span>' : ""}
      </div>
      <div class="problem-topics">${topicsHtml}</div>
    `;
    item.addEventListener("click", () => selectProblem(p.id));
    listEl.appendChild(item);
  }
}

function renderStats(stats) {
  const cards = document.getElementById("stat-cards");
  cards.innerHTML = `
    <div class="stat-card accent">
      <div class="stat-value">${stats.total}</div>
      <div class="stat-label">Total Problems</div>
    </div>
    <div class="stat-card solved">
      <div class="stat-value">${stats.solved}</div>
      <div class="stat-label">Solved</div>
    </div>
    <div class="stat-card">
      <div class="stat-value">${stats.percent}%</div>
      <div class="stat-label">Complete</div>
    </div>
  `;

  const bars = document.getElementById("diff-bars");
  bars.innerHTML = "";
  for (const d of DIFFICULTIES) {
    const info = stats.by_difficulty[d] || { total: 0, solved: 0 };
    const pct = info.total ? Math.round((info.solved / info.total) * 100) : 0;
    const row = document.createElement("div");
    row.className = "diff-bar-row";
    row.innerHTML = `
      <div class="diff-bar-label">
        <span>${d.replace("-", " ")}</span>
        <span>${info.solved}/${info.total}</span>
      </div>
      <div class="diff-bar-track">
        <div class="diff-bar-fill ${d}" style="width:${pct}%"></div>
      </div>
    `;
    bars.appendChild(row);
  }
}

function showHome() {
  currentProblemId = null;
  document.getElementById("home-view").style.display = "block";
  document.getElementById("problem-view").style.display = "none";
  renderProblemList();
  loadHome();
}

async function selectProblem(id) {
  currentProblemId = id;
  renderProblemList();

  const res = await fetch(`/api/problem/${id}`);
  const data = await res.json();

  document.getElementById("home-view").style.display = "none";
  document.getElementById("problem-view").style.display = "flex";

  document.getElementById("problem-title").textContent = data.meta.title;
  const topicsHtml = (data.meta.topics || [])
    .map(t => `<span class="topic-tag">${t}</span>`).join("");
  document.getElementById("problem-tags").innerHTML =
    `<span class="diff-dot ${data.meta.difficulty}"></span>${topicsHtml}`;

  document.getElementById("statement-md").innerHTML = marked.parse(data.problem_md);
  document.getElementById("results-panel").innerHTML = "";

  const status = data.progress?.solved ? "Solved" : "";
  const pill = document.getElementById("status-pill");
  pill.textContent = status;
  pill.className = status ? "pill-pass" : "";

  const attempts = data.progress?.attempts || 0;
  document.getElementById("attempts-pill").textContent =
    attempts ? `${attempts} attempt${attempts === 1 ? "" : "s"}` : "";

  if (!editor) {
    editor = CodeMirror.fromTextArea(document.getElementById("code-editor"), {
      mode: "text/x-c++src",
      theme: "dracula",
      lineNumbers: true,
      indentUnit: 4,
      tabSize: 4,
    });
  }
  editor.setValue(data.solution);
}

async function runTests() {
  if (!currentProblemId) return;
  const runBtn = document.getElementById("run-btn");
  const pill = document.getElementById("status-pill");
  const resultsPanel = document.getElementById("results-panel");

  runBtn.disabled = true;
  pill.textContent = "Running...";
  pill.className = "pill-running";
  resultsPanel.innerHTML = "";

  const content = editor.getValue();
  try {
    const res = await fetch(`/api/problem/${currentProblemId}/run`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ content }),
    });
    const result = await res.json();
    renderResults(result);
    if (result.progress) {
      const attempts = result.progress.attempts || 0;
      document.getElementById("attempts-pill").textContent =
        `${attempts} attempt${attempts === 1 ? "" : "s"}`;
      // refresh sidebar solved-state without a full reload
      const p = allProblems.find(p => p.id === currentProblemId);
      if (p) p.solved = result.progress.solved;
      renderProblemList();
    }
  } catch (e) {
    resultsPanel.textContent = "Failed to reach judge server: " + e;
  } finally {
    runBtn.disabled = false;
  }
}

function renderResults(result) {
  const pill = document.getElementById("status-pill");
  const panel = document.getElementById("results-panel");
  panel.innerHTML = "";

  if (result.status === "compile_error") {
    pill.textContent = "Compile Error";
    pill.className = "pill-fail";
    const pre = document.createElement("pre");
    pre.className = "case-detail";
    pre.textContent = result.compiler_output;
    panel.appendChild(pre);
    return;
  }

  if (result.status === "timeout" || result.status === "error") {
    pill.textContent = "Error";
    pill.className = "pill-fail";
    panel.textContent = result.message;
    return;
  }

  const passed = result.status === "pass";
  pill.textContent = passed ? "Solved" : "Failed";
  pill.className = passed ? "pill-pass" : "pill-fail";

  const summary = document.createElement("div");
  summary.className = "result-summary " + (passed ? "pass" : "fail");
  summary.textContent = `${result.passed_cases}/${result.total_cases} test cases passed ` +
    `(${result.passed_assertions}/${result.total_assertions} assertions)`;
  panel.appendChild(summary);

  for (const c of result.cases) {
    const row = document.createElement("div");
    row.className = "case-row " + (c.passed ? "pass" : "fail");
    row.textContent = c.name;
    row.style.cursor = "pointer";
    if (!c.passed) {
      const detail = document.createElement("div");
      detail.className = "case-detail";
      detail.textContent = c.detail;
      row.appendChild(detail);
    }
    panel.appendChild(row);
  }
}

document.getElementById("run-btn").addEventListener("click", runTests);
document.getElementById("back-btn").addEventListener("click", showHome);
document.getElementById("brand").addEventListener("click", showHome);

loadHome();
