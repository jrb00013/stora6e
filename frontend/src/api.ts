export type Category =
  | 'old'
  | 'large'
  | 'cache'
  | 'temp'
  | 'log'
  | 'duplicate'
  | 'empty_dir'
  | 'unknown';

export interface ScanEntry {
  path: string;
  size_bytes: number;
  size_human: string;
  modified_unix: number;
  category: Category;
  detail: string;
  selected_default: boolean;
}

export interface ScanStatus {
  status: 'idle' | 'running' | 'complete' | 'cancelled' | 'error';
  phase: string;
  current_path: string;
  files_scanned: number;
  dirs_scanned: number;
  bytes_scanned: number;
  bytes_scanned_human: string;
  percent: number;
  error?: string;
}

export interface ScanConfig {
  roots: string[];
  min_age_days: number;
  min_size_mb: number;
  scan_old: boolean;
  scan_large: boolean;
  scan_cache: boolean;
  scan_temp: boolean;
  scan_logs: boolean;
  scan_duplicates: boolean;
  scan_empty_dirs: boolean;
}

const API = '';

async function json<T>(path: string, init?: RequestInit): Promise<T> {
  const res = await fetch(`${API}${path}`, {
    headers: { 'Content-Type': 'application/json' },
    ...init,
  });
  if (!res.ok) {
    const text = await res.text();
    throw new Error(text || res.statusText);
  }
  return res.json() as Promise<T>;
}

export const api = {
  health: () => json<{ ok: boolean; app: string }>('/api/health'),
  defaults: () =>
    json<{ roots: string[]; min_age_days: number; min_size_mb: number }>('/api/defaults'),
  startScan: (config: Partial<ScanConfig>) =>
    json<{ ok: boolean }>('/api/scan/start', { method: 'POST', body: JSON.stringify(config) }),
  cancelScan: () => json<{ ok: boolean }>('/api/scan/cancel', { method: 'POST' }),
  scanStatus: () => json<ScanStatus>('/api/scan/status'),
  scanResults: (params?: { category?: string; q?: string; limit?: number; offset?: number }) => {
    const q = new URLSearchParams();
    if (params?.category) q.set('category', params.category);
    if (params?.q) q.set('q', params.q);
    if (params?.limit) q.set('limit', String(params.limit));
    if (params?.offset) q.set('offset', String(params.offset ?? 0));
    const qs = q.toString();
    return json<{
      total: number;
      returned: number;
      items: ScanEntry[];
      summary: { total_reclaimable_bytes: number; total_reclaimable_human: string };
    }>(`/api/scan/results${qs ? `?${qs}` : ''}`);
  },
  deletePaths: (paths: string[], useTrash = true) =>
    json<{ deleted: string[]; failed: { path: string; error: string }[] }>('/api/delete', {
      method: 'POST',
      body: JSON.stringify({ paths, use_trash: useTrash, confirm: true }),
    }),
};
