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
        
        if (saveBtn) {
            saveBtn.addEventListener('click', () => this.saveSettings());
        }
        
        if (cancelBtn) {
            cancelBtn.addEventListener('click', () => {
                window.location.href = '/';
            });
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

    showMessage(message, type) {
        const statusElement = document.getElementById('status');
        if (statusElement) {
            statusElement.textContent = message;
            statusElement.className = `status ${type === 'success' ? 'connected' : 'error'}`;
        }
    }
}

// Initialize the appropriate manager based on the current page
document.addEventListener('DOMContentLoaded', () => {
    if (window.location.pathname === '/settings') {
        new SettingsManager();
    } else {
        new AntennaSwitch();
    }
});
