let monacoEditor = null;
let originalSolutionText = "";
let currentProblemId = null;
let allProblems = [];
let activeDifficulties = new Set(); // empty = show all
let activeTypes = new Set();
let activeTopics = new Set();
let showTopics = localStorage.getItem("showTopics") !== "false";
let assistEnabled = localStorage.getItem("assistEnabled") !== "false";
let autosaveTimer = null;

const DIFFICULTIES = ["basic", "easy", "easy-medium", "medium", "hard", "expert"];
const TYPE_ICONS = {
  "implement": "\u2705",
  "from-scratch": "\ud83c\udfd7\ufe0f",
  "debug": "\ud83d\udc1b",
  "refactor": "\u267b\ufe0f",
  "performance": "\u26a1",
};
const TYPES = Object.keys(TYPE_ICONS);

function typeLabel(type) {
  return (TYPE_ICONS[type] || "") + " " + type;
}

function assistOptions(enabled) {
  return enabled
    ? {
        quickSuggestions: true,
        suggestOnTriggerCharacters: true,
        parameterHints: { enabled: true },
        hover: { enabled: true },
        wordBasedSuggestions: "currentDocument",
        suggest: { showKeywords: true, showSnippets: true },
      }
    : {
        quickSuggestions: false,
        suggestOnTriggerCharacters: false,
        parameterHints: { enabled: false },
        hover: { enabled: false },
        wordBasedSuggestions: "off",
        suggest: { showKeywords: false, showSnippets: false },
      };
}

// ---------- Monaco setup (loaded once, reused across problems) ----------
const monacoReady = new Promise((resolve) => {
  require.config({ paths: { vs: "https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.45.0/min/vs" } });
  require(["vs/editor/editor.main"], function () {
    monacoEditor = monaco.editor.create(document.getElementById("code-editor"), {
      value: "",
      language: "cpp",
      theme: "vs-dark",
      automaticLayout: false, // we call layout() manually on tab switch
      fontSize: 13,
      fontFamily: "'JetBrains Mono', monospace",
      minimap: { enabled: true },
      tabSize: 4,
      insertSpaces: true,
      scrollBeyondLastLine: false,
      ...assistOptions(assistEnabled),
    });
    monacoEditor.onDidChangeModelContent(() => scheduleAutosave());
    resolve();
  });
});

function scheduleAutosave() {
  if (!currentProblemId) return;
  const indicator = document.getElementById("save-indicator");
  indicator.textContent = "Editing...";
  indicator.classList.add("visible");
  clearTimeout(autosaveTimer);
  autosaveTimer = setTimeout(async () => {
    const content = monacoEditor.getValue();
    try {
      await fetch(`/api/problem/${currentProblemId}/solution`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ content }),
      });
      indicator.textContent = "Saved";
      setTimeout(() => indicator.classList.remove("visible"), 1200);
    } catch (e) {
      indicator.textContent = "Save failed";
    }
  }, 800);
}

function topicTagsHtml(topics) {
  if (!topics || !topics.length) return "";
  return topics.map(t => `<span class="topic-tag">${t}</span>`).join("");
}

// marked v12+ no longer supports a synchronous `highlight` option in
// setOptions (it's silently ignored) - fenced code blocks with a language
// tag render as <pre><code class="language-xxx"> by default though, which
// highlight.js can pick up directly. We call hljs on the rendered output
// right after inserting it - see selectProblem() below.

async function loadHome() {
  const [problemsRes, statsRes] = await Promise.all([
    fetch("/api/problems"), fetch("/api/stats")
  ]);
  allProblems = await problemsRes.json();
  const stats = await statsRes.json();

  renderDiffChips();
  renderTypeChips();
  renderTopicChips();
  updateMoreFiltersSummary();
  renderProblemGrid();
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
      updateMoreFiltersSummary();
      renderProblemGrid();
    });
    container.appendChild(chip);
  }
}

function updateMoreFiltersSummary() {
  const count = activeDifficulties.size + activeTypes.size + activeTopics.size;
  const countEl = document.getElementById("filters-count");
  countEl.textContent = count > 0 ? String(count) : "";
}

function renderTypeChips() {
  const container = document.getElementById("type-chips");
  container.innerHTML = "";
  for (const t of TYPES) {
    const chip = document.createElement("div");
    chip.className = "type-chip" + (activeTypes.has(t) ? " active" : "");
    chip.textContent = typeLabel(t);
    chip.addEventListener("click", () => {
      if (activeTypes.has(t)) activeTypes.delete(t);
      else activeTypes.add(t);
      renderTypeChips();
      updateMoreFiltersSummary();
      renderProblemGrid();
    });
    container.appendChild(chip);
  }
}

function renderTopicChips() {
  const container = document.getElementById("topic-chips");
  container.innerHTML = "";
  const allTopics = new Set();
  for (const p of allProblems) {
    for (const t of (p.topics || [])) allTopics.add(t);
  }
  for (const t of [...allTopics].sort()) {
    const chip = document.createElement("div");
    chip.className = "topic-chip" + (activeTopics.has(t) ? " active" : "");
    chip.textContent = t;
    chip.addEventListener("click", () => {
      if (activeTopics.has(t)) activeTopics.delete(t);
      else activeTopics.add(t);
      renderTopicChips();
      updateMoreFiltersSummary();
      renderProblemGrid();
    });
    container.appendChild(chip);
  }
}

function renderProblemGrid() {
  const grid = document.getElementById("problem-grid");
  grid.innerHTML = "";
  const filtered = allProblems.filter(p => {
    if (activeDifficulties.size > 0 && !activeDifficulties.has(p.difficulty)) return false;
    if (activeTypes.size > 0 && !activeTypes.has(p.type)) return false;
    if (activeTopics.size > 0 && !(p.topics || []).some(t => activeTopics.has(t))) return false;
    return true;
  });

  for (const p of filtered) {
    const card = document.createElement("div");
    card.className = `problem-card diff-border-${p.difficulty}${p.solved ? " is-solved" : ""}`;
    card.dataset.id = p.id;

    card.innerHTML = `
      <div class="problem-card-top">
        <span><span class="diff-dot ${p.difficulty}"></span>${p.title}</span>
        ${p.solved ? '<span class="solved-check">&#10003;</span>' : ""}
      </div>
      <div class="type-badge">${typeLabel(p.type)}</div>
      <div class="problem-topics ${showTopics ? "" : "hidden"}">${topicTagsHtml(p.topics)}</div>
      ${p.attempts ? `<div class="attempts-note">${p.attempts} attempt${p.attempts === 1 ? "" : "s"}</div>` : ""}
    `;
    card.addEventListener("click", () => selectProblem(p.id));
    grid.appendChild(card);
  }
}

function renderStats(stats) {
  const cards = document.getElementById("stat-cards");
  cards.innerHTML = `
    <div class="stat-card">
      <div class="stat-icon">&#128204;</div>
      <div class="stat-value">${stats.total}</div>
      <div class="stat-label">Total Problems</div>
    </div>
    <div class="stat-card c-success">
      <div class="stat-icon">&#9989;</div>
      <div class="stat-value">${stats.solved}</div>
      <div class="stat-label">Solved</div>
    </div>
    <div class="stat-card">
      <div class="stat-icon">&#128200;</div>
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
  loadHome();
}

async function selectProblem(id) {
  currentProblemId = id;

  const res = await fetch(`/api/problem/${id}`);
  const data = await res.json();

  document.getElementById("home-view").style.display = "none";
  document.getElementById("problem-view").style.display = "flex";

  document.getElementById("problem-title").textContent = data.meta.title;
  document.getElementById("problem-tags").innerHTML =
    `<span class="diff-dot ${data.meta.difficulty}"></span>` +
    `<span class="type-badge">${typeLabel(data.meta.type)}</span>` +
    (showTopics ? topicTagsHtml(data.meta.topics) : "");

  const statementEl = document.getElementById("statement-md");
  statementEl.innerHTML = marked.parse(data.problem_md);
  statementEl.querySelectorAll("pre code").forEach((block) => {
    hljs.highlightElement(block);
  });

  const testList = document.getElementById("test-names-list");
  testList.innerHTML = data.test_names.length
    ? data.test_names.map(n => `<div class="test-name-row">${n}</div>`).join("")
    : `<div class="results-empty">No test case names found.</div>`;

  document.getElementById("results-panel").innerHTML =
    `<div class="results-empty">Run your code to see results here.</div>`;
  document.getElementById("results-badge").className = "";
  document.getElementById("save-indicator").classList.remove("visible");

  const solved = data.progress?.solved;
  const pill = document.getElementById("status-pill");
  pill.textContent = solved ? "Solved" : "";
  pill.className = solved ? "pill-pass" : "";

  const attempts = data.progress?.attempts || 0;
  document.getElementById("attempts-pill").textContent =
    attempts ? `${attempts} attempt${attempts === 1 ? "" : "s"}` : "";

  switchTab("editor");

  await monacoReady;
  originalSolutionText = data.solution;
  monacoEditor.setValue(data.solution);
  setTimeout(() => monacoEditor.layout(), 0);
}

function switchTab(tabName) {
  document.querySelectorAll(".pane-tab").forEach(el =>
    el.classList.toggle("active", el.dataset.tab === tabName));
  document.querySelectorAll(".pane-content").forEach(el =>
    el.classList.toggle("active", el.dataset.content === tabName));
  if (tabName === "editor" && monacoEditor) setTimeout(() => monacoEditor.layout(), 0);
}

async function runTests() {
  if (!currentProblemId || !monacoEditor) return;
  const runBtn = document.getElementById("run-btn");
  const pill = document.getElementById("status-pill");
  const resultsPanel = document.getElementById("results-panel");

  runBtn.disabled = true;
  pill.textContent = "Running...";
  pill.className = "pill-running";
  resultsPanel.innerHTML = "";
  switchTab("results");

  const content = monacoEditor.getValue();
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
      const p = allProblems.find(p => p.id === currentProblemId);
      if (p) { p.solved = result.progress.solved; p.attempts = attempts; }
    }
  } catch (e) {
    resultsPanel.textContent = "Failed to reach judge server: " + e;
  } finally {
    runBtn.disabled = false;
  }
}

function renderResults(result) {
  const pill = document.getElementById("status-pill");
  const badge = document.getElementById("results-badge");
  const panel = document.getElementById("results-panel");
  panel.innerHTML = "";

  if (result.status === "compile_error") {
    pill.textContent = "Compile Error";
    pill.className = "pill-fail";
    badge.className = "fail";
    const pre = document.createElement("pre");
    pre.className = "case-detail";
    pre.textContent = result.compiler_output;
    panel.appendChild(pre);
    return;
  }

  if (result.status === "timeout" || result.status === "error") {
    pill.textContent = "Error";
    pill.className = "pill-fail";
    badge.className = "fail";
    panel.textContent = result.message;
    return;
  }

  if (result.status === "crashed") {
    pill.textContent = "Crashed";
    pill.className = "pill-fail";
    badge.className = "fail";
    const msg = document.createElement("div");
    msg.className = "result-summary fail";
    msg.textContent = result.message;
    panel.appendChild(msg);
    for (const c of (result.cases || [])) {
      const row = document.createElement("div");
      row.className = "case-row " + (c.passed ? "pass" : "fail");
      row.textContent = c.name + " (completed before the crash)";
      panel.appendChild(row);
    }
    return;
  }

  if (result.status !== "pass" && result.status !== "fail") {
    // Defensive fallback for any status this UI doesn't specifically know
    // about yet, so a judge-engine change never breaks silently here.
    pill.textContent = "Unknown result";
    pill.className = "pill-fail";
    badge.className = "fail";
    const pre = document.createElement("pre");
    pre.className = "case-detail";
    pre.textContent = result.raw_output || JSON.stringify(result, null, 2);
    panel.appendChild(pre);
    return;
  }

  const passed = result.status === "pass";
  pill.textContent = passed ? "Solved" : "Failed";
  pill.className = passed ? "pill-pass" : "pill-fail";
  badge.className = passed ? "pass" : "fail";

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

function randomProblem() {
  const filtered = allProblems.filter(p => {
    if (activeDifficulties.size > 0 && !activeDifficulties.has(p.difficulty)) return false;
    if (activeTypes.size > 0 && !activeTypes.has(p.type)) return false;
    if (activeTopics.size > 0 && !(p.topics || []).some(t => activeTopics.has(t))) return false;
    return true;
  });
  const pool = filtered.length ? filtered : allProblems;
  if (!pool.length) return;
  const pick = pool[Math.floor(Math.random() * pool.length)];
  selectProblem(pick.id);
}

document.getElementById("random-btn").addEventListener("click", randomProblem);

document.getElementById("run-btn").addEventListener("click", runTests);
document.getElementById("back-btn").addEventListener("click", showHome);
document.getElementById("brand").addEventListener("click", showHome);

document.getElementById("reset-btn").addEventListener("click", () => {
  if (!monacoEditor) return;
  if (confirm("Discard your current changes and reload the original stub?")) {
    monacoEditor.setValue(originalSolutionText);
  }
});

document.addEventListener("keydown", (e) => {
  if ((e.ctrlKey || e.metaKey) && e.key === "Enter" && currentProblemId) {
    e.preventDefault();
    runTests();
  }
});

document.querySelectorAll(".pane-tab").forEach(tab =>
  tab.addEventListener("click", () => switchTab(tab.dataset.tab)));

const topicsToggle = document.getElementById("topics-toggle");
topicsToggle.checked = showTopics;
topicsToggle.addEventListener("change", () => {
  showTopics = topicsToggle.checked;
  localStorage.setItem("showTopics", showTopics);
  document.querySelectorAll(".problem-topics").forEach(el =>
    el.classList.toggle("hidden", !showTopics));
  if (currentProblemId) {
    fetch(`/api/problem/${currentProblemId}`).then(r => r.json()).then(data => {
      document.getElementById("problem-tags").innerHTML =
        `<span class="diff-dot ${data.meta.difficulty}"></span>` +
        `<span class="type-badge">${typeLabel(data.meta.type)}</span>` +
        (showTopics ? topicTagsHtml(data.meta.topics) : "");
    });
  }
});

const assistToggle = document.getElementById("assist-toggle");
assistToggle.checked = assistEnabled;
assistToggle.addEventListener("change", async () => {
  assistEnabled = assistToggle.checked;
  localStorage.setItem("assistEnabled", assistEnabled);
  await monacoReady;
  monacoEditor.updateOptions(assistOptions(assistEnabled));
});

loadHome();
