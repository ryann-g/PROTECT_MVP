'use strict';
// Login throttling.
//
// HONEST LIMITATION: this is an in-memory counter. On Vercel each function
// instance keeps its own map, so a cold start resets it and an attacker
// spread across instances is slowed but not stopped. It is a speed bump, not
// a wall.
//
// Once the database is locked to the relay, the counters should move into
// the database so they are shared and cannot be reset by a cold start.

const WINDOW_MS = 15 * 60 * 1000; // 15 minutes
const MAX_ATTEMPTS = 8; // per key per window
const BLOCK_MS = 15 * 60 * 1000; // lockout length

const buckets = new Map();

function prune() {
  const cutoff = Date.now() - WINDOW_MS;
  buckets.forEach((bucket, key) => {
    if (bucket.blockedUntil && bucket.blockedUntil < Date.now()) {
      buckets.delete(key);
      return;
    }
    if (bucket.count === 0 && bucket.started < cutoff) buckets.delete(key);
  });
}

function bucketFor(key) {
  let bucket = buckets.get(key);
  if (!bucket) {
    bucket = { count: 0, started: Date.now(), blockedUntil: 0 };
    buckets.set(key, bucket);
  }
  return bucket;
}

// Returns { blocked: bool, retryAfterMs: number }
function isBlocked(key) {
  prune();
  const bucket = bucketFor(key);
  if (bucket.blockedUntil && bucket.blockedUntil > Date.now()) {
    return { blocked: true, retryAfterMs: bucket.blockedUntil - Date.now() };
  }
  return { blocked: false, retryAfterMs: 0 };
}

// Call on every failed attempt.
function recordFailure(key) {
  prune();
  const bucket = bucketFor(key);
  const cutoff = Date.now() - WINDOW_MS;
  if (bucket.started < cutoff) {
    bucket.count = 0;
    bucket.started = Date.now();
  }
  bucket.count += 1;
  if (bucket.count >= MAX_ATTEMPTS) {
    bucket.blockedUntil = Date.now() + BLOCK_MS;
  }
}

// Call on every successful attempt - clears the counter.
function recordSuccess(key) {
  buckets.delete(key);
}

function attemptsFor(key) {
  const bucket = buckets.get(key);
  return bucket ? bucket.count : 0;
}

function reset() {
  buckets.clear();
}

module.exports = { isBlocked, recordFailure, recordSuccess, attemptsFor, reset, MAX_ATTEMPTS, WINDOW_MS, BLOCK_MS };
