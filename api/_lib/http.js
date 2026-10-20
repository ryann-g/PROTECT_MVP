'use strict';
// Small HTTP helpers shared by the endpoints.

function json(res, status, body) {
  res.statusCode = status;
  res.setHeader('Content-Type', 'application/json; charset=utf-8');
  res.setHeader('Cache-Control', 'no-store');
  res.end(JSON.stringify(body));
}

function fail(res, status, message) {
  json(res, status, { ok: false, error: message });
}

function methodNotAllowed(res, allowed) {
  res.setHeader('Allow', allowed.join(', '));
  fail(res, 405, 'method not allowed');
}

// Vercel parses JSON bodies for us, but a raw body may also arrive.
async function body(req) {
  if (req.body && typeof req.body === 'object') return req.body;
  if (req.body && typeof req.body === 'string') {
    try { return JSON.parse(req.body); } catch (_) { return {}; }
  }
  if (typeof req.body === 'string' && req.body.length) {
    try { return JSON.parse(req.body); } catch (_) { return {}; }
  }
  return {};
}

function query(req) {
  const q = (req && req.query) || {};
  return q;
}

// Device and site ids go straight into a database path, so they are
// restricted to characters that cannot alter the path shape.
function safeId(value) {
  if (!value) return null;
  const s = String(value).trim();
  if (!/^[A-Za-z0-9_-]{1,64}$/.test(s)) return null;
  return s;
}

module.exports = { json, fail, methodNotAllowed, body, query, safeId };
