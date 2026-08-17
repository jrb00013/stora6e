import { useCallback, useEffect, useMemo, useState } from 'react';
import { api, Category, ScanConfig, ScanEntry, ScanStatus } from './api';

const CATEGORIES: { id: Category | ''; label: string }[] = [
  { id: '', label: 'All' },
  { id: 'cache', label: 'Cache' },
  { id: 'temp', label: 'Temp' },
  { id: 'old', label: 'Old' },
  { id: 'large', label: 'Large' },
  { id: 'log', label: 'Logs' },
  { id: 'duplicate', label: 'Duplicates' },
];

function formatDate(unix: number) {
  if (!unix) return '—';
  return new Date(unix * 1000).toLocaleDateString();
}

export default function App() {
  const [connected, setConnected] = useState(false);
  const [roots, setRoots] = useState('');
  const [minAge, setMinAge] = useState(90);
  const [minSizeMb, setMinSizeMb] = useState(50);
  const [flags, setFlags] = useState({
    scan_old: true,
    scan_large: true,
    scan_cache: true,
    scan_temp: true,
    scan_logs: true,
    scan_duplicates: false,
    scan_empty_dirs: false,
  });
  const [status, setStatus] = useState<ScanStatus | null>(null);
  const [items, setItems] = useState<ScanEntry[]>([]);
  const [total, setTotal] = useState(0);
  const [reclaimable, setReclaimable] = useState('—');
  const [selected, setSelected] = useState<Set<string>>(new Set());
  const [filterCat, setFilterCat] = useState<Category | ''>('');
  const [search, setSearch] = useState('');
  const [toast, setToast] = useState<{ msg: string; kind: 'ok' | 'error' } | null>(null);
  const [useTrash, setUseTrash] = useState(true);

  const showToast = (msg: string, kind: 'ok' | 'error' = 'ok') => {
    setToast({ msg, kind });
    setTimeout(() => setToast(null), 4000);
  };

  useEffect(() => {
    api
      .health()
      .then(() => setConnected(true))
      .catch(() => setConnected(false));
    api.defaults().then((d) => {
      setRoots(d.roots.join('\n'));
      setMinAge(d.min_age_days);
      setMinSizeMb(d.min_size_mb);
    });
  }, []);

  const refreshResults = useCallback(async () => {
    try {
      const data = await api.scanResults({
        category: filterCat || undefined,
        q: search || undefined,
        limit: 3000,
      });
      setItems(data.items);
      setTotal(data.total);
      setReclaimable(data.summary.total_reclaimable_human);
    } catch {
      /* idle */
    }
  }, [filterCat, search]);

  useEffect(() => {
    let timer: ReturnType<typeof setInterval> | undefined;
    const poll = async () => {
      try {
        const s = await api.scanStatus();
        const wasError = status?.status !== 'error' && s.status === 'error';
        setStatus(s);
        if (wasError) {
          showToast(s.error ? `Scan failed: ${s.error}` : 'Scan failed', 'error');
        }
        if (s.status === 'complete' || s.status === 'cancelled' || s.status === 'error') {
          await refreshResults();
        }
      } catch {
        setConnected(false);
      }
    };
    poll();
    timer = setInterval(poll, status?.status === 'running' ? 400 : 2000);
    return () => clearInterval(timer);
  }, [status?.status, refreshResults]);

  useEffect(() => {
    if (status?.status === 'complete') refreshResults();
  }, [filterCat, search, status?.status, refreshResults]);

  const scanning = status?.status === 'running';

  const config: Partial<ScanConfig> = useMemo(
    () => ({
      roots: roots
        .split('\n')
        .map((r) => r.trim())
        .filter(Boolean),
      min_age_days: minAge,
      min_size_mb: minSizeMb,
      ...flags,
    }),
    [roots, minAge, minSizeMb, flags]
  );

  const startScan = async () => {
    try {
      await api.startScan(config);
      setSelected(new Set());
      showToast('Scan started');
    } catch (e) {
      showToast(e instanceof Error ? e.message : 'Failed to start', 'error');
    }
  };

  const cancelScan = async () => {
    await api.cancelScan();
    showToast('Scan cancelled');
  };

  const toggleAll = (on: boolean) => {
    if (on) setSelected(new Set(items.map((i) => i.path)));
    else setSelected(new Set());
  };

  const selectDefaults = () => {
    setSelected(new Set(items.filter((i) => i.selected_default).map((i) => i.path)));
  };

  const togglePath = (path: string) => {
    setSelected((prev) => {
      const next = new Set(prev);
      if (next.has(path)) next.delete(path);
      else next.add(path);
      return next;
    });
  };

  const selectedSize = useMemo(() => {
    let bytes = 0;
    for (const it of items) {
      if (selected.has(it.path)) bytes += it.size_bytes;
    }
    return bytes;
  }, [items, selected]);

  const deleteSelected = async () => {
    if (selected.size === 0) return;
  if (!window.confirm(`Delete ${selected.size} item(s)?${useTrash ? ' (moved to Trash)' : ' (permanent)'}`))
      return;
    try {
      const res = await api.deletePaths([...selected], useTrash);
      showToast(`Deleted ${res.deleted.length}, failed ${res.failed.length}`);
      setSelected(new Set());
      await refreshResults();
    } catch (e) {
      showToast(e instanceof Error ? e.message : 'Delete failed', 'error');
    }
  };

  return (
    <div className="app">
      <header>
        <div className="logo">
          <div>
            <h1>
              stor<span>a6e</span>
            </h1>
            <p>Local storage cleanup — runs on your machine only</p>
          </div>
        </div>
        <span className="badge">{connected ? 'API connected' : 'API offline'}</span>
      </header>

      <div className="grid">
        <aside className="panel">
          <h2>Scan settings</h2>
          <label>Paths (one per line)</label>
          <textarea value={roots} onChange={(e) => setRoots(e.target.value)} spellCheck={false} />

          <label>Min age (days) — “old” files</label>
          <input
            type="number"
            min={1}
            value={minAge}
            onChange={(e) => setMinAge(Number(e.target.value))}
          />

          <label>Large file threshold (MB)</label>
          <input
            type="number"
            min={1}
            value={minSizeMb}
            onChange={(e) => setMinSizeMb(Number(e.target.value))}
          />

          <div className="checks">
            {(Object.keys(flags) as (keyof typeof flags)[]).map((key) => (
              <label key={key}>
                <input
                  type="checkbox"
                  checked={flags[key]}
                  onChange={(e) => setFlags((f) => ({ ...f, [key]: e.target.checked }))}
                />
                {key.replace('scan_', '').replace('_', ' ')}
              </label>
            ))}
          </div>

          <label>
            <input type="checkbox" checked={useTrash} onChange={(e) => setUseTrash(e.target.checked)} />{' '}
            Move to Trash (safer)
          </label>

          <div className="actions">
            <button className="btn-primary" onClick={startScan} disabled={scanning}>
              {scanning ? 'Scanning…' : 'Start scan'}
            </button>
            <button className="btn-secondary" onClick={cancelScan} disabled={!scanning}>
              Cancel
            </button>
          </div>
        </aside>

        <main className="panel">
          <h2>Results</h2>

          <div className="stats">
            <div className="stat">
              <div className="val">{total.toLocaleString()}</div>
              <div className="lbl">Matches</div>
            </div>
            <div className="stat">
              <div className="val">{reclaimable}</div>
              <div className="lbl">Reclaimable</div>
            </div>
            <div className="stat">
              <div className="val">{selected.size}</div>
              <div className="lbl">Selected</div>
            </div>
          </div>

          {status && (
            <div className="progress-wrap">
              <div className="progress-bar">
                <div className="progress-fill" style={{ width: `${status.percent}%` }} />
              </div>
              <div className="progress-meta">
                <span>{status.status} — {status.phase}</span>
                <span>{status.files_scanned.toLocaleString()} files · {status.bytes_scanned_human}</span>
              </div>
              {status.current_path && (
                <div className="progress-meta">
                  <span style={{ opacity: 0.7 }}>{status.current_path}</span>
                </div>
              )}
              {status.status === 'error' && status.error && (
                <div className="progress-meta" style={{ color: '#e05d5d' }}>
                  <span>Scan error: {status.error}</span>
                </div>
              )}
            </div>
          )}

          <div className="toolbar">
            <input
              placeholder="Filter paths…"
              value={search}
              onChange={(e) => setSearch(e.target.value)}
            />
            <div className="chips">
              {CATEGORIES.map((c) => (
                <button
                  key={c.id || 'all'}
                  type="button"
                  className={`chip ${filterCat === c.id ? 'active' : ''}`}
                  onClick={() => setFilterCat(c.id)}
                >
                  {c.label}
                </button>
              ))}
            </div>
          </div>

          <div className="actions" style={{ marginBottom: '0.75rem' }}>
            <button type="button" className="btn-secondary" onClick={() => toggleAll(true)}>
              Select page
            </button>
            <button type="button" className="btn-secondary" onClick={selectDefaults}>
              Select safe defaults
            </button>
            <button type="button" className="btn-secondary" onClick={() => toggleAll(false)}>
              Clear
            </button>
            <button
              type="button"
              className="btn-danger"
              onClick={deleteSelected}
              disabled={selected.size === 0}
            >
              Delete selected ({selected.size})
              {selectedSize > 0 && ` · ${(selectedSize / 1024 / 1024).toFixed(1)} MB`}
            </button>
          </div>

          <div className="table-wrap">
            {items.length === 0 ? (
              <div className="empty">
                {scanning ? 'Scanning filesystem…' : 'No results yet. Configure paths and start a scan.'}
              </div>
            ) : (
              <table>
                <thead>
                  <tr>
                    <th style={{ width: 32 }} />
                    <th>Path</th>
                    <th>Category</th>
                    <th>Size</th>
                    <th>Modified</th>
                  </tr>
                </thead>
                <tbody>
                  {items.map((row) => (
                    <tr key={`${row.path}-${row.category}`}>
                      <td>
                        <input
                          type="checkbox"
                          checked={selected.has(row.path)}
                          onChange={() => togglePath(row.path)}
                        />
                      </td>
                      <td className="path" title={row.path}>
                        {row.path}
                        {row.detail && (
                          <div style={{ color: 'var(--muted)', fontSize: '0.68rem' }}>{row.detail}</div>
                        )}
                      </td>
                      <td>
                        <span className={`cat ${row.category}`}>{row.category}</span>
                      </td>
                      <td>{row.size_human}</td>
                      <td>{formatDate(row.modified_unix)}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            )}
          </div>
        </main>
      </div>

      {toast && <div className={`toast ${toast.kind}`}>{toast.msg}</div>}
    </div>
  );
}
