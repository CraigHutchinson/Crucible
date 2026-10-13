// Distance dolly: sweeps a unit from 6 to 150 px and back while it turns, comparing the
// distance ladder (hard cut and cross-fade) against always-highest detail, frame by frame.
// Usage: node lod_dolly.mjs <lab dir> <three.js package dir> <output dir> [--video]
// Writes dolly.json (per-frame size, rung, error against the reference and the jump a
// switch causes) and, with --video,
// dolly.mp4: panels reference | hard cut | cross-fade, real pixels enlarged 2x.
import { chromium } from "playwright";
import { createServer } from "node:http";
import { readFile, mkdir, writeFile } from "node:fs/promises";
import { extname, join } from "node:path";
import { spawn } from "node:child_process";

const [labDir, threeDir, outDir] = process.argv.slice(2);
const video = process.argv.includes("--video");
await mkdir(join(outDir, "dolly-frames"), { recursive: true });
const types = { ".html": "text/html", ".mjs": "text/javascript", ".js": "text/javascript" };
const server = createServer(async (req, res) => {
    try {
        const path = join(labDir, decodeURIComponent(new URL(req.url, "http://x").pathname).replace(/^\/$/, "/index.html"));
        res.writeHead(200, { "content-type": types[extname(path)] ?? "application/octet-stream" });
        res.end(await readFile(path));
    } catch { res.writeHead(404); res.end(); }
}).listen(0);
const browser = await chromium.launch({ args: ["--use-angle=swiftshader", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist"] });
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
page.on("console", (m) => { if (m.type() === "error") console.error("page:", m.text()); });
await page.route("https://cdn.jsdelivr.net/npm/three@0.170.0/**", async (route) => {
    const rel = new URL(route.request().url()).pathname.replace("/npm/three@0.170.0/", "");
    route.fulfill({ body: await readFile(join(threeDir, rel)), contentType: "text/javascript" });
});
await page.route("https://fonts.googleapis.com/**", (route) => route.abort());
await page.goto(`http://localhost:${server.address().port}/index.html?capture`);
await page.waitForFunction(() => window.lab, null, { timeout: 120000 });

const frames = Number(process.env.FRAMES ?? 300);
const result = { rows: [] };
const chunk = 30; // render in chunks so frame images stream out
let rung = null;
const all = await page.evaluate(([n, keep]) => {
    const { rows, images, ladder, capRung } = window.lab.dollyStudy({ frames: n, keepFrames: keep });
    return { rows, images, ladder, capRung };
}, [frames, video]);
result.rows = all.rows;
result.ladder = all.ladder;
result.capRung = all.capRung;
for (let i = 0; i < all.images.length; ++i) {
    await writeFile(join(outDir, "dolly-frames", `f${String(i).padStart(4, "0")}.png`), Buffer.from(all.images[i].split(",")[1], "base64"));
}
await writeFile(join(outDir, "dolly.json"), JSON.stringify(result, null, 1));
const summary = (key) => {
    const values = result.rows.map((r) => r[key]).filter((v) => v !== undefined).sort((a, b) => a - b);
    return { max: values.at(-1), p99: values[Math.floor(values.length * 0.99)], mean: values.reduce((t, v) => t + v, 0) / values.length };
};
const switches = result.rows.filter((r, i) => i > 0 && r.rung !== result.rows[i - 1].rung).map((r) => ({ frame: r.frame, size: +r.size.toFixed(1), rung: r.rung }));
console.log(JSON.stringify({ switches, errorCut: summary("errorCut"), errorFade: summary("errorFade"), jumpCut: summary("jumpCut"), jumpFade: summary("jumpFade"), motion: summary("motion") }, null, 1));
if (video) {
    await new Promise((resolve, reject) => spawn("ffmpeg", ["-y", "-loglevel", "error", "-framerate", "30", "-i",
        join(outDir, "dolly-frames", "f%04d.png"), "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "18", join(outDir, "dolly.mp4")],
        { stdio: "inherit" }).on("exit", (code) => code ? reject(new Error("ffmpeg " + code)) : resolve()));
}
await browser.close();
server.close();
