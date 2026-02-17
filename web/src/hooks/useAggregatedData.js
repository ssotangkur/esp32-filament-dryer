import { useMemo } from 'react';

const SECOND_MS = 1000;
const MINUTE_MS = 60 * SECOND_MS;
const HOUR_MS = 60 * MINUTE_MS;

function aggregateBySecond(data) {
  return data;
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

  return result.sort((a, b) => a.x - b.x);
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

  return result.sort((a, b) => a.x - b.x);
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

  return result.sort((a, b) => a.x - b.x);
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
