class AntennaSwitch {
    constructor() {
        this.ws = null;
        this.reconnectInterval = null;
        this.antennaNames = [];
        this.currentState = { radio1: 0, radio2: 0 };
        
        this.init();
    }

    init() {
        this.loadAntennaNames();
        this.setupEventListeners();
        this.connectWebSocket();
    }

    async loadAntennaNames() {
        try {
            const response = await fetch('/api/antennas');
            this.antennaNames = await response.json();
            this.updateAntennaNames();
        } catch (error) {
            console.error('Failed to load antenna names:', error);
        }
    }

    updateAntennaNames(names = null) {
        // If names are provided (from WebSocket), update our local copy
        if (names) {
            this.antennaNames = names;
        }
        
        // Update the UI with current antenna names
        for (let i = 1; i <= 6; i++) {
            const element = document.getElementById(`antenna-${i}`);
            if (element && this.antennaNames[i-1]) {
                element.textContent = this.antennaNames[i-1];
            }
        }
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
                    this.updateState(data.radio1, data.radio2);
                } else if (data.type === 'antennaNames') {
                    this.updateAntennaNames(data.names);
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
            if (antenna > 0) {
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
        this.setupEventListeners();
    }

    async loadAntennaNames() {
        try {
            const response = await fetch('/api/antennas');
            const antennaNames = await response.json();
            
            for (let i = 0; i < 6; i++) {
                const input = document.getElementById(`antenna-${i}`);
                if (input) {
                    input.value = antennaNames[i] || `Antenna ${i + 1}`;
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
    }

    async saveSettings() {
        const antennaNames = {};
        
        for (let i = 0; i < 6; i++) {
            const input = document.getElementById(`antenna-${i}`);
            if (input) {
                antennaNames[i] = input.value.trim() || `Antenna ${i + 1}`;
            }
        }

        const hostnameInput = document.getElementById('mdns-hostname');
        const hostname = hostnameInput ? hostnameInput.value.trim() : 'antenna';

        try {
            // Save antenna names
            const antennaResponse = await fetch('/api/antennas', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(antennaNames)
            });

            // Save hostname
            const hostnameResponse = await fetch('/api/hostname', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ hostname: hostname })
            });

            if (antennaResponse.ok && hostnameResponse.ok) {
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
                } else {
                    const errorText = await hostnameResponse.text();
                    this.showMessage(`Failed to save hostname: ${errorText}`, 'error');
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
    if (window.location.pathname === '/settings') {
        new SettingsManager();
    } else if (window.location.pathname === '/status') {
        new StatusViewer();
    } else {
        new AntennaSwitch();
    }
});
