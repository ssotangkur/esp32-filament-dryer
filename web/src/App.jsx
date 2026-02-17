import React, { useState, useEffect, useRef, useCallback } from 'react';
import { ThemeProvider, createTheme } from '@mui/material/styles';
import { Container, Typography, Paper, Box, ToggleButtonGroup, ToggleButton } from '@mui/material';
import { LineChart } from '@mui/x-charts/LineChart';
import { useAggregatedData } from './hooks/useAggregatedData';

const theme = createTheme();

const DEVICE_IP = import.meta.env.VITE_DEVICE_IP || '192.168.2.18';
const WS_PORT = import.meta.env.VITE_WS_PORT || '3000';

function formatTime(timestamp) {
  return new Date(timestamp).toLocaleTimeString();
}

function App() {
  const [version, setVersion] = useState('Loading...');
  const [airData, setAirData] = useState([]);
  const [heaterData, setHeaterData] = useState([]);
  const [granularity, setGranularity] = useState('second');
  const wsRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);
  const reconnectAttempts = useRef(0);

  const aggregatedAirData = useAggregatedData(airData, granularity);
  const aggregatedHeaterData = useAggregatedData(heaterData, granularity);

  const handleGranularityChange = (event, newGranularity) => {
    if (newGranularity !== null) {
      setGranularity(newGranularity);
    }
  };

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
          setAirData(prev => [...prev, { x: point.timestamp, y: point.temperature }]);
        } else if (point.sensor === 'heater') {
          setHeaterData(prev => [...prev, { x: point.timestamp, y: point.temperature }]);
        }
      });
    };

    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };

    ws.onclose = (event) => {
      console.log('WebSocket closed:', event.code, event.reason);
      wsRef.current = null;
      
      if (reconnectAttempts.current < 5) {
        reconnectAttempts.current++;
        const delay = Math.min(1000 * Math.pow(2, reconnectAttempts.current), 30000);
        console.log(`Reconnecting in ${delay}ms... attempt ${reconnectAttempts.current}`);
        reconnectTimeoutRef.current = setTimeout(connectWebSocket, delay);
      }
    };
  }, []);

  useEffect(() => {
    fetch(`/api/version`)
      .then(res => res.text())
      .then(setVersion)
      .catch(err => setVersion('Error loading version'));

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
          <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', mb: 2 }}>
            <Typography variant="h6">Time Granularity</Typography>
            <ToggleButtonGroup
              value={granularity}
              exclusive
              onChange={handleGranularityChange}
              size="small"
            >
              <ToggleButton value="second">1s</ToggleButton>
              <ToggleButton value="minute">1m</ToggleButton>
              <ToggleButton value="10min">10m</ToggleButton>
              <ToggleButton value="hour">1h</ToggleButton>
            </ToggleButtonGroup>
          </Box>
        </Paper>

        <Paper sx={{ p: 2, mb: 2 }}>
          <Typography variant="h6" gutterBottom>Air Temperature</Typography>
          {aggregatedAirData.length > 0 ? (
            <LineChart
              xAxis={[{ dataKey: 'x', type: 'number', valueFormatter: formatTime }]}
              series={[{ dataKey: 'y', label: 'Air (°C)' }]}
              dataset={aggregatedAirData}
              width={500}
              height={300}
            />
          ) : (
            <Typography>Waiting for air sensor data...</Typography>
          )}
          <Typography variant="body2" color="text.secondary">
            Data points: {airData.length} (aggregated to {aggregatedAirData.length})
          </Typography>
        </Paper>

        <Paper sx={{ p: 2 }}>
          <Typography variant="h6" gutterBottom>Heater Temperature</Typography>
          {aggregatedHeaterData.length > 0 ? (
            <LineChart
              xAxis={[{ dataKey: 'x', type: 'number', valueFormatter: formatTime }]}
              series={[{ dataKey: 'y', label: 'Heater (°C)' }]}
              dataset={aggregatedHeaterData}
              width={500}
              height={300}
            />
          ) : (
            <Typography>Waiting for heater sensor data...</Typography>
          )}
          <Typography variant="body2" color="text.secondary">
            Data points: {heaterData.length} (aggregated to {aggregatedHeaterData.length})
          </Typography>
        </Paper>
      </Container>
    </ThemeProvider>
  );
}

export default App;
