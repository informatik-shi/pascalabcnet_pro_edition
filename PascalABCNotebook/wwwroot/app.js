const state = { token: '', notebook: null, saveTimer: null, busy: false, editVersion: 0 };
const $ = (id) => document.getElementById(id);
const examples = {
  graphabc: "uses GraphABC;\nSetWindowSize(480, 300);\nClearWindow(clWhite);\nBrush.Color := clSkyBlue;\nFillCircle(240, 150, 90);",
  graphwpf: "uses GraphWPF;\nWindow.SetSize(480, 300);\nBrush.Color := Colors.CornflowerBlue;\nFillCircle(240, 150, 90);",
  graph3d: "uses Graph3D;\nWindow.SetSize(480, 360);\nSphere(0, 0, 0, 2, Colors.CornflowerBlue);",
  turtleabc: "uses TurtleABC;\nfor var i := 1 to 36 do\nbegin\n  Forw(120);\n  Turn(170);\nend;",
  plotml: "uses PlotML;\nPlot.LineGraph([0.0, 1.0, 2.0], [0.0, 1.0, 4.0]);",
  wpf: "uses WPF;\nMainWindow.Width := 480;\nMainWindow.Height := 300;\nvar panel := Panels.DockPanel.AsMainContent;\nvar button := Controls.Button('Привет, WPF');\npanel.Children.Add(button);"
};

async function api(method, path, body) {
  const headers = {};
  if (body !== undefined) headers['Content-Type'] = 'application/json';
  if (method !== 'GET') headers['X-Notebook-Token'] = state.token;
  const response = await fetch(path, {
    method, headers, body: body === undefined ? undefined : JSON.stringify(body)
  });
  const data = await response.json();
  if (!response.ok) throw new Error(data.error || `Ошибка HTTP ${response.status}`);
  return data;
}

function showError(error) {
  $('saveStatus').textContent = `Ошибка: ${error.message || error}`;
  $('saveStatus').style.color = '#ff9b9b';
}

function newCell(code = '') {
  return { id: crypto.randomUUID(), code, output: '', imageUrl: null, succeeded: true };
}

function setBusy(value) {
  state.busy = value;
  document.querySelectorAll('#notebookView button, #notebookView textarea, #notebookTitle, #exampleSelect')
    .forEach(element => { element.disabled = value; });
}

async function showHome() {
  if (state.notebook && !state.busy) await saveNow();
  state.notebook = null;
  location.hash = '';
  $('notebookView').hidden = true;
  $('homeView').hidden = false;
  const list = $('notebookList');
  list.replaceChildren();
  const notebooks = await api('GET', '/api/notebooks');
  if (!notebooks.length) {
    const empty = document.createElement('p');
    empty.className = 'emptyList';
    empty.textContent = 'Пока нет тетрадок. Создайте первую выше.';
    list.append(empty);
  }
  for (const notebook of notebooks) {
    const row = document.createElement('button');
    row.type = 'button';
    row.className = 'notebookLink';
    const badge = document.createElement('span');
    badge.className = 'badge' + (notebook.language === 'spython' ? ' py' : '');
    badge.textContent = notebook.language === 'spython' ? 'SPython' : 'Pascal';
    const title = document.createElement('strong');
    title.textContent = notebook.title;
    const time = document.createElement('time');
    time.textContent = new Date(notebook.updatedUtc).toLocaleString('ru-RU');
    row.append(badge, title, time);
    row.addEventListener('click', () => openNotebook(notebook.id));
    list.append(row);
  }
}

async function createNotebook(language) {
  const notebook = await api('POST', '/api/notebooks', { language });
  await openNotebook(notebook.id);
}

async function openNotebook(id) {
  if (state.notebook && !state.busy) await saveNow();
  state.notebook = await api('GET', `/api/notebooks/${id}`);
  state.editVersion = 0;
  location.hash = id;
  $('homeView').hidden = true;
  $('notebookView').hidden = false;
  $('languageBadge').textContent = state.notebook.language === 'spython' ? 'SPython' : 'PascalABC.NET';
  $('languageBadge').className = 'badge' + (state.notebook.language === 'spython' ? ' py' : '');
  $('notebookTitle').value = state.notebook.title;
  $('saveStatus').textContent = 'Сохранено';
  $('saveStatus').style.color = '';
  $('exampleSelect').hidden = state.notebook.language !== 'pascal';
  renderCells();
}

function scheduleSave() {
  state.editVersion++;
  $('saveStatus').textContent = 'Есть несохранённые изменения';
  $('saveStatus').style.color = '';
  clearTimeout(state.saveTimer);
  state.saveTimer = setTimeout(() => saveNow().catch(showError), 650);
}

async function saveNow() {
  clearTimeout(state.saveTimer);
  if (!state.notebook) return;
  const snapshot = structuredClone(state.notebook);
  const version = state.editVersion;
  await api('PUT', `/api/notebooks/${snapshot.id}`, snapshot);
  if (version === state.editVersion) $('saveStatus').textContent = 'Сохранено';
}

function renderCells() {
  const container = $('cells');
  container.replaceChildren();
  state.notebook.cells.forEach((cell, index) => {
    const article = document.createElement('article');
    article.className = 'cell';
    article.dataset.cellId = cell.id;
    const header = document.createElement('div');
    header.className = 'cellHeader';
    const label = document.createElement('span');
    label.className = 'cellNumber';
    label.textContent = `Ячейка ${index + 1}`;
    const actions = document.createElement('div');
    actions.className = 'cellActions';
    actions.append(
      action('▶ Выполнить', 'runButton', () => runCell(index)),
      action('＋ Ниже', '', () => insertCell(index + 1)),
      action('↑', 'moveButton', () => moveCell(index, -1)),
      action('↓', 'moveButton', () => moveCell(index, 1)),
      action('Удалить', 'deleteButton', () => removeCell(index))
    );
    header.append(label, actions);
    const input = document.createElement('textarea');
    input.className = 'code';
    input.setAttribute('aria-label', `Код ячейки ${index + 1}`);
    input.spellcheck = false;
    input.value = cell.code;
    input.placeholder = state.notebook.language === 'pascal'
      ? "writeln('Привет, мир!');"
      : "print('Привет, мир!')";
    input.addEventListener('input', () => { cell.code = input.value; scheduleSave(); });
    input.addEventListener('keydown', event => {
      if (event.key === 'Tab') {
        event.preventDefault();
        insertAtCursor(input, '    ');
      } else if (event.key === 'Enter' && event.ctrlKey) {
        event.preventDefault(); runCell(index);
      } else if (event.key === 'Enter' && event.shiftKey) {
        event.preventDefault(); runCell(index).then(() => focusNext(index));
      } else if (event.key === 'Enter' && state.notebook.language === 'spython') {
        event.preventDefault();
        const line = input.value.slice(0, input.selectionStart).split('\n').at(-1);
        const indent = line.match(/^\s*/)[0] + (line.trimEnd().endsWith(':') ? '    ' : '');
        insertAtCursor(input, '\n' + indent);
      }
    });
    const output = document.createElement('div');
    output.className = 'cellOutput';
    article.append(header, input, output);
    container.append(article);
    renderOutput(output, cell);
  });
}

function insertAtCursor(input, text) {
  const position = input.selectionStart;
  input.setRangeText(text, input.selectionStart, input.selectionEnd, 'end');
  input.selectionStart = input.selectionEnd = position + text.length;
  input.dispatchEvent(new Event('input'));
}

function action(label, className, callback) {
  const button = document.createElement('button');
  button.type = 'button';
  button.textContent = label;
  button.className = className;
  button.addEventListener('click', callback);
  return button;
}

function renderOutput(element, cell) {
  element.replaceChildren();
  element.className = 'cellOutput' + (cell.succeeded ? '' : ' error');
  if (cell.output) {
    const title = document.createElement('div');
    title.className = 'outputLabel';
    title.textContent = cell.succeeded ? 'Вывод' : 'Ошибка';
    const pre = document.createElement('pre');
    pre.className = 'outputText';
    pre.textContent = cell.output;
    element.append(title, pre);
  }
  if (cell.imageUrl) {
    const title = document.createElement('div');
    title.className = 'outputLabel';
    title.textContent = 'Графика';
    const image = document.createElement('img');
    image.src = cell.imageUrl;
    image.alt = 'Графический результат ячейки';
    element.append(title, image);
  }
}

async function runCell(index) {
  if (state.busy) return;
  try { await saveNow(); }
  catch (error) { showError(error); return; }
  setBusy(true);
  const article = $('cells').children[index];
  article.classList.add('running');
  $('saveStatus').textContent = `Выполняется ячейка ${index + 1}…`;
  try {
    const result = await api('POST', `/api/notebooks/${state.notebook.id}/run`, { index });
    const cell = state.notebook.cells[index];
    cell.output = result.output;
    cell.imageUrl = result.imageUrl;
    cell.succeeded = result.succeeded;
    renderOutput(article.querySelector('.cellOutput'), cell);
    $('saveStatus').textContent = result.succeeded
      ? `Готово за ${(result.durationMs / 1000).toFixed(1)} с`
      : `Ошибка в ячейке ${index + 1}`;
  } catch (error) { showError(error); }
  finally { article.classList.remove('running'); setBusy(false); }
}

async function runAll() {
  for (let index = 0; index < state.notebook.cells.length; index++) {
    await runCell(index);
    if (!state.notebook.cells[index].succeeded) break;
  }
}

function focusNext(index) {
  if (index + 1 >= state.notebook.cells.length) insertCell(index + 1);
  $('cells').children[index + 1]?.querySelector('textarea')?.focus();
}

function insertCell(index, code = '') {
  if (state.busy) return;
  state.notebook.cells.splice(index, 0, newCell(code));
  renderCells(); scheduleSave();
  $('cells').children[index]?.querySelector('textarea')?.focus();
}

function removeCell(index) {
  if (state.busy || state.notebook.cells.length === 1) return;
  state.notebook.cells.splice(index, 1);
  renderCells(); scheduleSave();
}

function moveCell(index, direction) {
  const destination = index + direction;
  if (state.busy || destination < 0 || destination >= state.notebook.cells.length) return;
  const [cell] = state.notebook.cells.splice(index, 1);
  state.notebook.cells.splice(destination, 0, cell);
  renderCells(); scheduleSave();
}

async function start() {
  state.token = (await api('GET', '/api/session')).token;
  $('homeButton').addEventListener('click', () => showHome().catch(showError));
  $('createPascal').addEventListener('click', () => createNotebook('pascal').catch(showError));
  $('createSPython').addEventListener('click', () => createNotebook('spython').catch(showError));
  $('notebookTitle').addEventListener('input', event => {
    state.notebook.title = event.target.value; scheduleSave();
  });
  $('notebookTitle').addEventListener('blur', () => {
    if (!state.notebook.title.trim()) {
      state.notebook.title = 'Без названия';
      $('notebookTitle').value = state.notebook.title;
    }
    saveNow().catch(showError);
  });
  $('addCellTop').addEventListener('click', () => insertCell(0));
  $('addCellBottom').addEventListener('click', () => insertCell(state.notebook.cells.length));
  $('runAll').addEventListener('click', () => runAll().catch(showError));
  $('exampleSelect').addEventListener('change', event => {
    const code = examples[event.target.value];
    if (code) insertCell(state.notebook.cells.length, code);
    event.target.value = '';
  });
  const id = location.hash.slice(1);
  if (id) {
    try { await openNotebook(id); return; } catch { /* Deleted or invalid notebook. */ }
  }
  await showHome();
}

start().catch(error => {
  $('connectionStatus').textContent = `Ошибка подключения: ${error.message}`;
});
