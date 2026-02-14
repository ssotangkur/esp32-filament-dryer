import React, { useState, useEffect, useRef, useCallback } from 'react';
import { ThemeProvider, createTheme } from '@mui/material/styles';
import { Container, Typography, Paper, Box } from '@mui/material';
import { LineChart } from '@mui/x-charts/LineChart';

const theme = createTheme();

const SLIDING_WINDOW_SIZE = 50;

const DEVICE_IP = import.meta.env.VITE_DEVICE_IP || '192.168.2.18';
const WS_PORT = import.meta.env.VITE_WS_PORT || '3000';

function App() {
  const [version, setVersion] = useState('Loading...');
  const [airData, setAirData] = useState([]);
  const [heaterData, setHeaterData] = useState([]);
  const wsRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);
  const reconnectAttempts = useRef(0);

  const connectWebSocket = useCallback(() => {
    const wsUrl = `ws://${DEVICE_IP}:${WS_PORT}/ws/sensor-data`;
    console.log('WebSocket URL:', wsUrl);
    
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      console.log('WebSocket already connected');
      return;
    }

    const ws = new WebSocket(wsUrl);
    wsRef.current = ws;

    ws.onopen = () => {
      console.log('WebSocket connected');
      reconnectAttempts.current = 0;
      ws.send('get_data');
    };

    ws.onmessage = (event) => {
      console.log('WebSocket message:', event.data);
      const data = JSON.parse(event.data);
      data.forEach(point => {
        if (point.sensor === 'air') {
          setAirData(prev => [...prev.slice(-SLIDING_WINDOW_SIZE + 1), { x: point.timestamp, y: point.temperature }]);
        } else if (point.sensor === 'heater') {
          setHeaterData(prev => [...prev.slice(-SLIDING_WINDOW_SIZE + 1), { x: point.timestamp, y: point.temperature }]);
        }
      });
    };

    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };

    ws.onclose = (event) => {
      console.log('WebSocket closed:', event.code, event.reason);
      wsRef.current = null;
      
      // Auto-reconnect with exponential backoff
      if (reconnectAttempts.current < 5) {
        reconnectAttempts.current++;
        const delay = Math.min(1000 * Math.pow(2, reconnectAttempts.current), 30000);
        console.log(`Reconnecting in ${delay}ms... attempt ${reconnectAttempts.current}`);
        reconnectTimeoutRef.current = setTimeout(connectWebSocket, delay);
      }
    };
  }, []);

  useEffect(() => {
    // Fetch firmware version
    fetch(`/api/version`)
      .then(res => res.text())
      .then(setVersion)
      .catch(err => setVersion('Error loading version'));

    // Connect WebSocket
    connectWebSocket();

    return () => {
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, [connectWebSocket]);
 
  return (
    <ThemeProvider theme={theme}>
      <Container maxWidth="md" sx={{ mt: 4 }}>
        <Typography variant="h4" component="h1" gutterBottom>
          ESP32 Filament Dryer
        </Typography>

        <Paper sx={{ p: 2, mb: 2 }}>
          <Typography variant="h6">Firmware Version</Typography>
          <Typography>{version}</Typography>
        </Paper>

        <Paper sx={{ p: 2, mb: 2 }}>
          <Typography variant="h6" gutterBottom>Air Temperature</Typography>
          {airData.length > 0 ? (
            <LineChart
              xAxis={[{ dataKey: 'x', type: 'number' }]}
              series={[{ dataKey: 'y', label: 'Air (°C)' }]}
              dataset={airData}
              width={500}
              height={300}
            />
          ) : (
            <Typography>Waiting for air sensor data...</Typography>
          )}
        </Paper>

        <Paper sx={{ p: 2 }}>
          <Typography variant="h6" gutterBottom>Heater Temperature</Typography>
          {heaterData.length > 0 ? (
            <LineChart
              xAxis={[{ dataKey: 'x', type: 'number' }]}
              series={[{ dataKey: 'y', label: 'Heater (°C)' }]}
              dataset={heaterData}
              width={500}
              height={300}
            />
          ) : (
            <Typography>Waiting for heater sensor data...</Typography>
          )}
        </Paper>
      </Container>
    </ThemeProvider>
  );
}

export default App;
