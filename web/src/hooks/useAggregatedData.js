import { useMemo } from 'react';

const MAX_DATA_POINTS = 100;

const SECOND_MS = 1000;
const MINUTE_MS = 60 * SECOND_MS;
const HOUR_MS = 60 * MINUTE_MS;

function aggregateBySecond(data) {
  if (!data || data.length === 0) return [];
  
  if (data.length <= MAX_DATA_POINTS) {
    return data;
  }
  
  const step = Math.ceil(data.length / MAX_DATA_POINTS);
  const result = [];
  for (let i = data.length - 1; i >= 0 && result.length < MAX_DATA_POINTS; i -= step) {
    result.unshift(data[i]);
  }
  return result;
}

function aggregateByMinute(data) {
  if (!data || data.length === 0) return [];

  const buckets = new Map();

  data.forEach(point => {
    const bucketKey = Math.floor(point.x / MINUTE_MS) * MINUTE_MS;
    
    if (!buckets.has(bucketKey)) {
      buckets.set(bucketKey, { sum: 0, count: 0 });
    }
    
    const bucket = buckets.get(bucketKey);
    bucket.sum += point.y;
    bucket.count++;
  });

  const result = [];
  buckets.forEach((value, key) => {
    result.push({
      x: key,
      y: value.sum / value.count
    });
  });

  const sorted = result.sort((a, b) => a.x - b.x);
  
  if (sorted.length <= MAX_DATA_POINTS) {
    return sorted;
  }
  
  const step = Math.ceil(sorted.length / MAX_DATA_POINTS);
  const capped = [];
  for (let i = sorted.length - 1; i >= 0 && capped.length < MAX_DATA_POINTS; i -= step) {
    capped.unshift(sorted[i]);
  }
  return capped;
}

function aggregateBy10Min(data) {
  if (!data || data.length === 0) return [];

  const bucketSize = 10 * MINUTE_MS;
  const buckets = new Map();

  data.forEach(point => {
    const bucketKey = Math.floor(point.x / bucketSize) * bucketSize;
    
    if (!buckets.has(bucketKey)) {
      buckets.set(bucketKey, { sum: 0, count: 0 });
    }
    
    const bucket = buckets.get(bucketKey);
    bucket.sum += point.y;
    bucket.count++;
  });

  const result = [];
  buckets.forEach((value, key) => {
    result.push({
      x: key,
      y: value.sum / value.count
    });
  });

  const sorted = result.sort((a, b) => a.x - b.x);
  
  if (sorted.length <= MAX_DATA_POINTS) {
    return sorted;
  }
  
  const step = Math.ceil(sorted.length / MAX_DATA_POINTS);
  const capped = [];
  for (let i = sorted.length - 1; i >= 0 && capped.length < MAX_DATA_POINTS; i -= step) {
    capped.unshift(sorted[i]);
  }
  return capped;
}

function aggregateByHour(data) {
  if (!data || data.length === 0) return [];

  const buckets = new Map();

  data.forEach(point => {
    const bucketKey = Math.floor(point.x / HOUR_MS) * HOUR_MS;
    
    if (!buckets.has(bucketKey)) {
      buckets.set(bucketKey, { sum: 0, count: 0 });
    }
    
    const bucket = buckets.get(bucketKey);
    bucket.sum += point.y;
    bucket.count++;
  });

  const result = [];
  buckets.forEach((value, key) => {
    result.push({
      x: key,
      y: value.sum / value.count
    });
  });

  const sorted = result.sort((a, b) => a.x - b.x);
  
  if (sorted.length <= MAX_DATA_POINTS) {
    return sorted;
  }
  
  const step = Math.ceil(sorted.length / MAX_DATA_POINTS);
  const capped = [];
  for (let i = sorted.length - 1; i >= 0 && capped.length < MAX_DATA_POINTS; i -= step) {
    capped.unshift(sorted[i]);
  }
  return capped;
}

export function useAggregatedData(data, granularity) {
  return useMemo(() => {
    switch (granularity) {
      case 'second':
        return aggregateBySecond(data);
      case 'minute':
        return aggregateByMinute(data);
      case '10min':
        return aggregateBy10Min(data);
      case 'hour':
        return aggregateByHour(data);
      default:
        return aggregateBySecond(data);
    }
  }, [data, granularity]);
}
