#!/usr/bin/env node

const http = require("http");
const { URL } = require("url");

const port = Number(process.argv[2] || 8787);
const samples = [];
let lastSummaryAt = 0;

function addSample(sample) {
  sample.receivedAt = Date.now();
  samples.push(sample);
  while (samples.length > 300) samples.shift();
}

function summary() {
  const metricSamples = samples.filter((item) => item.type === "metrics");
  const recent = metricSamples.slice(-8);
  if (!recent.length) return { count: samples.length, metricCount: 0 };
  const avg = (key) =>
    recent.reduce((sum, item) => sum + Number(item[key] || 0), 0) /
    recent.length;
  const latest = recent[recent.length - 1];
  const frameTimes = recent
    .map((item) => Number(item.frameMs || 0))
    .filter((value) => Number.isFinite(value))
    .sort((a, b) => a - b);
  const percentile = (p) => {
    if (!frameTimes.length) return 0;
    const index = Math.min(frameTimes.length - 1, Math.floor((frameTimes.length - 1) * p));
    return frameTimes[index];
  };
  return {
    count: samples.length,
    metricCount: metricSamples.length,
    latest,
    window: recent.length,
    liveFps: Number(latest.fps || 0),
    p50FrameMs: percentile(0.50),
    p95FrameMs: percentile(0.95),
    recentAvgFps: avg("fps"),
    recentAvgUploadPixels: avg("uploadPixels"),
    recentAvgQuads: avg("quads"),
  };
}

const server = http.createServer((req, res) => {
  const url = new URL(req.url, `http://${req.headers.host || "localhost"}`);
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "content-type");
  if (req.method === "OPTIONS") {
    res.writeHead(204);
    res.end();
    return;
  }
  if (url.pathname === "/metrics") {
    let body = "";
    req.on("data", (chunk) => {
      body += chunk;
      if (body.length > 32768) req.destroy();
    });
    req.on("end", () => {
      try {
        const sample = req.method === "POST" && body
          ? JSON.parse(body)
          : Object.fromEntries(url.searchParams.entries());
        addSample(sample);
        const now = Date.now();
        if (now - lastSummaryAt > 2000) {
          lastSummaryAt = now;
          console.log(JSON.stringify(summary()));
        }
        res.writeHead(204);
        res.end();
      } catch (error) {
        res.writeHead(400, { "content-type": "text/plain" });
        res.end(String(error && error.message ? error.message : error));
      }
    });
    return;
  }
  if (url.pathname === "/latest") {
    res.writeHead(200, { "content-type": "application/json" });
    res.end(JSON.stringify(summary(), null, 2));
    return;
  }
  res.writeHead(200, { "content-type": "text/plain" });
  res.end("tizen render benchmark metrics server\n");
});

server.listen(port, "0.0.0.0", () => {
  console.log(`metrics server listening on http://0.0.0.0:${port}`);
});
