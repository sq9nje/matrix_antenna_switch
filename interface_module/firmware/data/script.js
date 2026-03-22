// Theme management
function initTheme() {
    const saved = localStorage.getItem('theme');
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    applyTheme(saved || (prefersDark ? 'dark' : 'light'));
}

function applyTheme(theme) {
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem('theme', theme);
    const btn = document.getElementById('theme-toggle');
    if (btn) {
        btn.textContent = theme === 'dark' ? '\u2600' : '\u263E';
        btn.title = theme === 'dark' ? 'Switch to light mode' : 'Switch to dark mode';
    }
}

function toggleTheme() {
    const current = document.documentElement.getAttribute('data-theme') || 'light';
    applyTheme(current === 'dark' ? 'light' : 'dark');
}

class AntennaSwitch {
    constructor() {
        this.ws = null;
        this.reconnectInterval = null;
        this.antennas = [];
        this.currentState = { radio1: 0, radio2: 0 };
        this.operationMode = { antennaSwapping: false, singleRadioMode: false };
        
        this.init();
    }

    async init() {
        this.loadAntennaNames();
        await this.loadOperationMode();
        this.setupEventListeners();
        this.connectWebSocket();
    }

    async loadAntennaNames() {
        try {
            const response = await fetch('/api/antennas');
            this.antennas = await response.json();
            this.updateAntennaNames();
        } catch (error) {
            console.error('Failed to load antenna names:', error);
        }
    }

    async loadOperationMode() {
        try {
            const response = await fetch('/api/operation-mode');
            const data = await response.json();
            
            this.operationMode = {
                antennaSwapping: data.antennaSwapping || false,
                singleRadioMode: data.singleRadioMode || false
            };
            
            this.updateSingleRadioMode();
        } catch (error) {
            console.error('Failed to load operation mode:', error);
        }
    }

    updateSingleRadioMode() {
        const antennaGrid = document.querySelector('.antenna-grid');
        if (antennaGrid) {
            if (this.operationMode.singleRadioMode) {
                antennaGrid.classList.add('single-radio');
            } else {
                antennaGrid.classList.remove('single-radio');
            }
        }
    }

    updateAntennaNames(antennasData = null) {
        if (antennasData) {
            this.antennas = antennasData;
        }

        const radio1Col = document.getElementById('radio1-column');
        const namesCol = document.getElementById('names-column');
        const radio2Col = document.getElementById('radio2-column');

        // Remove existing dynamic antenna elements (keep h2 and disconnected button/placeholder)
        radio1Col.querySelectorAll('.antenna-btn:not(.disconnected)').forEach(el => el.remove());
        namesCol.querySelectorAll('.antenna-name:not(:first-of-type)').forEach(el => el.remove());
        radio2Col.querySelectorAll('.antenna-btn:not(.disconnected)').forEach(el => el.remove());

        for (let i = 1; i <= 6; i++) {
            const antenna = this.antennas[i - 1];
            if (!antenna || !antenna.name || antenna.name.trim() === '') continue;

            const btn1 = document.createElement('button');
            btn1.className = 'antenna-btn';
            btn1.dataset.radio = '0';
            btn1.dataset.antenna = String(i);
            btn1.textContent = String(i);
            radio1Col.appendChild(btn1);

            const nameDiv = document.createElement('div');
            nameDiv.className = 'antenna-name';
            nameDiv.id = `antenna-${i}`;

            const nameSpan = document.createElement('span');
            nameSpan.className = 'antenna-name-text';
            nameSpan.textContent = antenna.name;
            nameDiv.appendChild(nameSpan);

            const bands = antenna.bands || [];
            if (bands.length > 0) {
                const badgesDiv = document.createElement('div');
                badgesDiv.className = 'band-badges';
                bands.forEach(band => {
                    const badge = document.createElement('span');
                    badge.className = 'band-badge band-' + band.toLowerCase().replace(/\s/g, '');
                    badge.textContent = band;
                    badgesDiv.appendChild(badge);
                });
                nameDiv.appendChild(badgesDiv);
            }

            namesCol.appendChild(nameDiv);

            const btn2 = document.createElement('button');
            btn2.className = 'antenna-btn';
            btn2.dataset.radio = '1';
            btn2.dataset.antenna = String(i);
            btn2.textContent = String(i);
            radio2Col.appendChild(btn2);
        }

        const rowCount = namesCol.children.length;
        [radio1Col, namesCol, radio2Col].forEach(col => {
            col.style.gridRow = 'span ' + rowCount;
        });

        this.setupEventListeners();
        this.updateState(this.currentState.radio1, this.currentState.radio2);
    }

    setupEventListeners() {
        document.querySelectorAll('.antenna-btn').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const radio = parseInt(e.target.dataset.radio);
                const antenna = parseInt(e.target.dataset.antenna);
                this.selectAntenna(radio, antenna);
            });
        });
    }

    connectWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.hostname}:81`;
        
        this.ws = new WebSocket(wsUrl);
        
        this.ws.onopen = () => {
            console.log('WebSocket connected');
            this.updateStatus('Connected', 'connected');
            if (this.reconnectInterval) {
                clearInterval(this.reconnectInterval);
                this.reconnectInterval = null;
            }
        };
        
        this.ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                if (data.type === 'state') {
                    // Update operation mode if received
                    if (data.hasOwnProperty('singleRadioMode')) {
                        this.operationMode.singleRadioMode = data.singleRadioMode;
                        this.updateSingleRadioMode();
                    }
                    this.updateState(data.radio1, data.radio2);
                } else if (data.type === 'antennaNames') {
                    this.updateAntennaNames(data.antennas);
                }
            } catch (error) {
                console.error('Error parsing WebSocket message:', error);
            }
        };
        
        this.ws.onclose = () => {
            console.log('WebSocket disconnected');
            this.updateStatus('Disconnected - Reconnecting...', 'error');
            this.startReconnect();
        };
        
        this.ws.onerror = (error) => {
            console.error('WebSocket error:', error);
            this.updateStatus('Connection Error', 'error');
        };
    }

    startReconnect() {
        if (!this.reconnectInterval) {
            this.reconnectInterval = setInterval(() => {
                console.log('Attempting to reconnect...');
                this.connectWebSocket();
            }, 3000);
        }
    }

    selectAntenna(radio, antenna) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            const message = {
                type: 'select',
                radio: radio,
                antenna: antenna
            };
            this.ws.send(JSON.stringify(message));
        } else {
            this.updateStatus('Not connected to switch', 'error');
        }
    }

    updateState(radio1, radio2) {
        this.currentState = { radio1, radio2 };
        
        // Update button states
        document.querySelectorAll('.antenna-btn').forEach(btn => {
            const radio = parseInt(btn.dataset.radio);
            const antenna = parseInt(btn.dataset.antenna);
            
            btn.classList.remove('active', 'disabled');
            
            // Mark active buttons
            if ((radio === 0 && antenna === radio1) || (radio === 1 && antenna === radio2)) {
                btn.classList.add('active');
            }
            
            // Disable buttons that are selected by the other radio (except disconnected)
            if (antenna > 0 && !this.operationMode.antennaSwapping) {
                if ((radio === 0 && antenna === radio2) || (radio === 1 && antenna === radio1)) {
                    btn.classList.add('disabled');
                }
            }
        });
    }

    updateStatus(message, className = '') {
        const statusElement = document.getElementById('status');
        if (statusElement) {
            statusElement.textContent = message;
            statusElement.className = 'status ' + className;
        }
    }
}

// Settings page functionality
class SettingsManager {
    constructor() {
        if (window.location.pathname === '/settings') {
            this.init();
        }
    }

    async init() {
        await this.loadAntennaNames();
        await this.loadHostname();
        await this.loadOperationMode();
        await this.loadOTRSPSettings();
        this.setupEventListeners();
    }

    async loadAntennaNames() {
        try {
            const response = await fetch('/api/antennas');
            const antennas = await response.json();

            for (let i = 0; i < 6; i++) {
                const input = document.getElementById(`antenna-${i}`);
                if (input) {
                    input.value = antennas[i].name || '';
                }

                const bands = antennas[i].bands || [];
                const container = document.querySelector(`.band-checkboxes[data-antenna="${i}"]`);
                if (container) {
                    container.querySelectorAll('input[type="checkbox"]').forEach(cb => {
                        cb.checked = bands.includes(cb.value);
                    });
                }
            }
        } catch (error) {
            console.error('Failed to load antenna names:', error);
            this.showMessage('Failed to load antenna names', 'error');
        }
    }

    async loadHostname() {
        try {
            const response = await fetch('/api/hostname');
            const data = await response.json();
            
            const hostnameInput = document.getElementById('mdns-hostname');
            if (hostnameInput) {
                hostnameInput.value = data.hostname || 'antenna';
            }
        } catch (error) {
            console.error('Failed to load hostname:', error);
            this.showMessage('Failed to load hostname', 'error');
        }
    }

    async loadOperationMode() {
        try {
            const response = await fetch('/api/operation-mode');
            const data = await response.json();
            
            const antennaSwappingInput = document.getElementById('antenna-swapping');
            const singleRadioModeInput = document.getElementById('single-radio-mode');
            
            if (antennaSwappingInput) {
                antennaSwappingInput.checked = data.antennaSwapping || false;
            }
            
            if (singleRadioModeInput) {
                singleRadioModeInput.checked = data.singleRadioMode || false;
            }
        } catch (error) {
            console.error('Failed to load operation mode:', error);
            this.showMessage('Failed to load operation mode', 'error');
        }
    }

    async loadOTRSPSettings() {
        try {
            const response = await fetch('/api/otrsp/status');
            const data = await response.json();

            const otrspEnabled = document.getElementById('otrsp-enabled');
            const otrspSerialEnabled = document.getElementById('otrsp-serial-enabled');

            if (otrspEnabled) otrspEnabled.checked = data.enabled || false;
            if (otrspSerialEnabled) otrspSerialEnabled.checked = data.serialEnabled || false;
        } catch (error) {
            console.error('Failed to load OTRSP settings:', error);
        }
    }

    setupEventListeners() {
        const saveBtn = document.getElementById('save-btn');
        const cancelBtn = document.getElementById('cancel-btn');
        const rebootBtn = document.getElementById('reboot-btn');
        const resetNetworkBtn = document.getElementById('reset-network-btn');
        
        if (saveBtn) {
            saveBtn.addEventListener('click', () => this.saveSettings());
        }
        
        if (cancelBtn) {
            cancelBtn.addEventListener('click', () => {
                window.location.href = '/';
            });
        }
        
        if (rebootBtn) {
            rebootBtn.addEventListener('click', () => this.rebootDevice());
        }
        
        if (resetNetworkBtn) {
            resetNetworkBtn.addEventListener('click', () => this.resetNetworkSettings());
        }

        const exportBtn = document.getElementById('export-btn');
        const importBtn = document.getElementById('import-btn');
        const importFile = document.getElementById('import-file');

        if (exportBtn) {
            exportBtn.addEventListener('click', () => this.exportSettings());
        }

        if (importFile) {
            importFile.addEventListener('change', () => {
                if (importBtn) {
                    importBtn.disabled = !importFile.files.length;
                }
            });
        }

        if (importBtn) {
            importBtn.addEventListener('click', () => this.importSettings());
        }
    }

    async saveSettings() {
        const antennaData = {};

        for (let i = 0; i < 6; i++) {
            const input = document.getElementById(`antenna-${i}`);
            const name = input ? input.value.trim() : '';
            const bands = [];
            const container = document.querySelector(`.band-checkboxes[data-antenna="${i}"]`);
            if (container) {
                container.querySelectorAll('input[type="checkbox"]:checked').forEach(cb => {
                    bands.push(cb.value);
                });
            }
            antennaData[i] = { name, bands };
        }

        const hostnameInput = document.getElementById('mdns-hostname');
        const hostname = hostnameInput ? hostnameInput.value.trim() : 'antenna';

        const antennaSwappingInput = document.getElementById('antenna-swapping');
        const singleRadioModeInput = document.getElementById('single-radio-mode');
        
        const operationMode = {
            antennaSwapping: antennaSwappingInput ? antennaSwappingInput.checked : false,
            singleRadioMode: singleRadioModeInput ? singleRadioModeInput.checked : false
        };

        try {
            // Save antenna names and bands
            const antennaResponse = await fetch('/api/antennas', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(antennaData)
            });

            // Save hostname
            const hostnameResponse = await fetch('/api/hostname', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ hostname: hostname })
            });

            // Save operation mode
            const operationModeResponse = await fetch('/api/operation-mode', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(operationMode)
            });

            // Save OTRSP settings
            const otrspEnabledInput = document.getElementById('otrsp-enabled');
            const otrspSerialEnabledInput = document.getElementById('otrsp-serial-enabled');
            const otrspResponse = await fetch('/api/otrsp/enable', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    enabled: otrspEnabledInput ? otrspEnabledInput.checked : false,
                    serialEnabled: otrspSerialEnabledInput ? otrspSerialEnabledInput.checked : false
                })
            });

            if (antennaResponse.ok && hostnameResponse.ok && operationModeResponse.ok && otrspResponse.ok) {
                const hostnameText = await hostnameResponse.text();
                if (hostnameText.includes('Restart required')) {
                    this.showMessage('Settings saved! Restart device to apply hostname changes.', 'success');
                } else {
                    this.showMessage('Settings saved successfully!', 'success');
                }
                setTimeout(() => {
                    window.location.href = '/';
                }, 2000);
            } else {
                if (!antennaResponse.ok) {
                    this.showMessage('Failed to save antenna names', 'error');
                } else if (!hostnameResponse.ok) {
                    const errorText = await hostnameResponse.text();
                    this.showMessage(`Failed to save hostname: ${errorText}`, 'error');
                } else {
                    this.showMessage('Failed to save operation mode', 'error');
                }
            }
        } catch (error) {
            console.error('Error saving settings:', error);
            this.showMessage('Failed to save settings', 'error');
        }
    }

    async rebootDevice() {
        if (confirm('Are you sure you want to reboot the device? This will interrupt any active connections.')) {
            try {
                this.showMessage('Rebooting device...', 'warning');
                const response = await fetch('/api/reboot', {
                    method: 'POST'
                });
                
                if (response.ok) {
                    this.showMessage('Device is rebooting. Please wait...', 'warning');
                    setTimeout(() => {
                        window.location.reload();
                    }, 10000);
                } else {
                    this.showMessage('Failed to reboot device', 'error');
                }
            } catch (error) {
                console.error('Error rebooting device:', error);
                this.showMessage('Error rebooting device', 'error');
            }
        }
    }

    async resetNetworkSettings() {
        if (confirm('Are you sure you want to reset network settings? This will erase all WiFi credentials and the device will restart in AP mode for reconfiguration.')) {
            try {
                this.showMessage('Resetting network settings...', 'warning');
                const response = await fetch('/api/reset-network', {
                    method: 'POST'
                });
                
                if (response.ok) {
                    this.showMessage('Network settings reset. Device will restart in AP mode.', 'warning');
                    setTimeout(() => {
                        alert('Network settings have been reset. The device will now restart in AP mode. Look for the "AntennaSwitch" WiFi network to reconfigure.');
                        window.location.reload();
                    }, 3000);
                } else {
                    this.showMessage('Failed to reset network settings', 'error');
                }
            } catch (error) {
                console.error('Error resetting network settings:', error);
                this.showMessage('Error resetting network settings', 'error');
            }
        }
    }

    async exportSettings() {
        try {
            const response = await fetch('/api/settings/export');
            const blob = await response.blob();
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'settings.json';
            a.click();
            URL.revokeObjectURL(url);
        } catch (error) {
            console.error('Failed to export settings:', error);
            this.showMessage('Failed to export settings', 'error');
        }
    }

    async importSettings() {
        const fileInput = document.getElementById('import-file');
        if (!fileInput || !fileInput.files.length) return;

        if (!confirm('This will overwrite all current settings. Continue?')) return;

        try {
            const text = await fileInput.files[0].text();
            JSON.parse(text); // validate JSON

            const response = await fetch('/api/settings/import', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: text
            });

            if (response.ok) {
                this.showMessage('Settings imported successfully! Reloading...', 'success');
                setTimeout(() => window.location.reload(), 2000);
            } else {
                const errorText = await response.text();
                this.showMessage(`Import failed: ${errorText}`, 'error');
            }
        } catch (error) {
            console.error('Failed to import settings:', error);
            this.showMessage('Failed to import settings: invalid JSON file', 'error');
        }
    }

    showMessage(message, type) {
        const statusElement = document.getElementById('status');
        if (statusElement) {
            statusElement.textContent = message;
            let className = 'status';
            if (type === 'success') {
                className += ' connected';
            } else if (type === 'warning') {
                className += ' warning';
            } else {
                className += ' error';
            }
            statusElement.className = className;
        }
    }
}

// Status page functionality
class StatusViewer {
    constructor() {
        if (window.location.pathname === '/status') {
            this.init();
        }
    }

    async init() {
        await this.loadStatus();
        this.setupEventListeners();
        
        // Auto-refresh every 5 seconds
        setInterval(() => this.loadStatus(), 5000);
    }

    setupEventListeners() {
        const refreshBtn = document.getElementById('refresh-btn');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', () => this.loadStatus());
        }
    }

    async loadStatus() {
        try {
            const response = await fetch('/api/status');
            const data = await response.json();
            
            this.updateNetworkInfo(data);
            this.updateFirmwareInfo(data);
            this.updateDeviceInfo(data);
            this.updateAntennaStatus(data);
            
        } catch (error) {
            console.error('Failed to load status:', error);
            this.showError('Failed to load status information');
        }
    }

    updateNetworkInfo(data) {
        document.getElementById('wifi-ssid').textContent = data.ssid || 'Not connected';
        document.getElementById('ip-address').textContent = data.ip || 'N/A';
        document.getElementById('gateway').textContent = data.gateway || 'N/A';
        document.getElementById('subnet').textContent = data.subnet || 'N/A';
        document.getElementById('dns').textContent = data.dns || 'N/A';
        document.getElementById('mac-address').textContent = data.macAddress || 'N/A';
        document.getElementById('hostname').textContent = data.hostname + '.local' || 'N/A';
        
        // Format signal strength
        const signalElement = document.getElementById('signal-strength');
        if (data.rssi !== undefined) {
            let signalText = `${data.rssi} dBm`;
            if (data.rssi > -50) {
                signalText += ' (Excellent)';
            } else if (data.rssi > -60) {
                signalText += ' (Good)';
            } else if (data.rssi > -70) {
                signalText += ' (Fair)';
            } else {
                signalText += ' (Weak)';
            }
            signalElement.textContent = signalText;
        } else {
            signalElement.textContent = 'N/A';
        }
    }

    updateFirmwareInfo(data) {
        document.getElementById('firmware-version').textContent = data.firmwareVersion || 'Unknown';
        
        // Format build time if available
        const buildTime = data.buildTime || 'Unknown';
        if (buildTime !== 'Unknown' && buildTime.length === 15) {
            // Format YYYYMMDD_HHMMSS to readable format
            const year = buildTime.substr(0, 4);
            const month = buildTime.substr(4, 2);
            const day = buildTime.substr(6, 2);
            const hour = buildTime.substr(9, 2);
            const minute = buildTime.substr(11, 2);
            const second = buildTime.substr(13, 2);
            
            const formattedTime = `${year}-${month}-${day} ${hour}:${minute}:${second}`;
            document.getElementById('build-time').textContent = formattedTime;
        } else {
            document.getElementById('build-time').textContent = buildTime;
        }
    }

    updateDeviceInfo(data) {
        document.getElementById('chip-model').textContent = data.chipModel || 'N/A';
        document.getElementById('chip-revision').textContent = data.chipRevision || 'N/A';
        document.getElementById('cpu-freq').textContent = data.cpuFreqMHz ? `${data.cpuFreqMHz} MHz` : 'N/A';
        
        // Format memory information
        const freeHeap = data.freeHeap || 0;
        const totalHeap = data.totalHeap || 0;
        const usedHeap = totalHeap - freeHeap;
        const usagePercent = totalHeap > 0 ? Math.round((usedHeap / totalHeap) * 100) : 0;
        
        document.getElementById('free-heap').textContent = `${this.formatBytes(freeHeap)} (${100 - usagePercent}% free)`;
        document.getElementById('total-heap').textContent = `${this.formatBytes(totalHeap)} (${usagePercent}% used)`;
        
        // Format uptime
        const uptime = data.uptime || 0;
        document.getElementById('uptime').textContent = this.formatUptime(uptime);
    }

    updateAntennaStatus(data) {
        const radio1 = data.currentRadio1 || 0;
        const radio2 = data.currentRadio2 || 0;
        
        document.getElementById('radio1-antenna').textContent = radio1 === 0 ? 'Disconnected' : `Antenna ${radio1}`;
        document.getElementById('radio2-antenna').textContent = radio2 === 0 ? 'Disconnected' : `Antenna ${radio2}`;
    }

    formatBytes(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    formatUptime(seconds) {
        const days = Math.floor(seconds / (24 * 60 * 60));
        const hours = Math.floor((seconds % (24 * 60 * 60)) / (60 * 60));
        const minutes = Math.floor((seconds % (60 * 60)) / 60);
        const secs = Math.floor(seconds % 60);
        
        if (days > 0) {
            return `${days}d ${hours}h ${minutes}m ${secs}s`;
        } else if (hours > 0) {
            return `${hours}h ${minutes}m ${secs}s`;
        } else if (minutes > 0) {
            return `${minutes}m ${secs}s`;
        } else {
            return `${secs}s`;
        }
    }

    showError(message) {
        // Update all loading elements to show error
        const loadingElements = document.querySelectorAll('#status [id]:not([id="refresh-btn"])');
        loadingElements.forEach(element => {
            if (element.textContent === 'Loading...') {
                element.textContent = 'Error';
            }
        });
    }
}

// Initialize the appropriate manager based on the current page
document.addEventListener('DOMContentLoaded', () => {
    initTheme();
    const themeBtn = document.getElementById('theme-toggle');
    if (themeBtn) themeBtn.addEventListener('click', toggleTheme);

    if (window.location.pathname === '/settings') {
        new SettingsManager();
    } else if (window.location.pathname === '/status') {
        new StatusViewer();
    } else {
        new AntennaSwitch();
    }
});
