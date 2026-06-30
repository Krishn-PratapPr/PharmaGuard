window.PHARMA_GUARD_CONFIG_READY = (async function() {
  try {
    const response = await fetch('./.env');
    if (!response.ok) throw new Error(`Unable to load .env (${response.status})`);
    const text = await response.text();
    const config = {};
    text.split(/\r?\n/).forEach(line => {
      const trimmed = line.trim();
      if (!trimmed || trimmed.startsWith('#')) return;
      const [key, ...rest] = trimmed.split('=');
      if (!key) return;
      const value = rest.join('=').trim();
      config[key.trim()] = value.replace(/^"|"$/g, '').replace(/^\'|\'$/g, '');
    });
    window.PHARMA_GUARD_CONFIG = config;
    return config;
  } catch (err) {
    console.error('Config loader failed:', err);
    window.PHARMA_GUARD_CONFIG = null;
    throw err;
  }
})();
