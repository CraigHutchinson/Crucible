// Detail-transition (LOD pop) study: renders each pair of neighbouring detail levels at a
// range of projected sizes and records how different they look at that size.
// Usage: node lod_study.mjs <lab dir> <three.js package dir> <output dir>
import { chromium } from "playwright";
import { createServer } from "node:http";
import { readFile, mkdir, writeFile } from "node:fs/promises";
import { extname, join } from "node:path";

const [labDir, threeDir, outDir] = process.argv.slice(2);
await mkdir(outDir, { recursive: true });
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

let pairs = [];
if (process.env.PAIRS) pairs = JSON.parse(process.env.PAIRS);
else for (let s = 0; s < 4; ++s) for (let p = 0; p < 4; ++p) {
    if (s < 3) pairs.push([[s, p], [s + 1, p]]);
    if (p < 3) pairs.push([[s, p], [s, p + 1]]);
}
const sizes = (process.env.SIZES ?? "6,8,12,16,24,32,48,64,96,128").split(",").map(Number);
const views = [[0.3, 0.45], [1.9, 0.45], [3.5, 0.45], [5.1, 0.2]];
if (process.env.GLASS === "0") await page.evaluate(() => window.lab.set({ glass: false }));
const rows = await page.evaluate(([p, s, v]) => window.lab.popStudy(p, s, v), [pairs, sizes, views]);
await writeFile(join(outDir, "pop-study.json"), JSON.stringify({ sizes, views, rows }, null, 1));
for (const r of rows) console.log(`S${r.from[0]}P${r.from[1]}->S${r.to[0]}P${r.to[1]} ${String(r.size).padStart(4)}px mean ${r.mean.toFixed(4)} worst ${r.worst.toFixed(4)} p95 ${r.p95.toFixed(3)} iou ${r.iou.toFixed(3)}`);
await browser.close();
server.close();
