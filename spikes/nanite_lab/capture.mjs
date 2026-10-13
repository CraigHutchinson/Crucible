// Headless captures of the nanite lab (build_lab.sh output) for review evidence.
// Usage: node capture.mjs <lab dir> <three.js package dir> <output dir> [--video]
// CDN requests for three.js are served from the local package; fonts fall back to system faces.
import { chromium } from "playwright";
import { createServer } from "node:http";
import { readFile, mkdir, writeFile } from "node:fs/promises";
import { extname, join } from "node:path";
import { spawn } from "node:child_process";

const [labDir, threeDir, outDir] = process.argv.slice(2);
const video = process.argv.includes("--video");
await mkdir(outDir, { recursive: true });
const types = { ".html": "text/html", ".mjs": "text/javascript", ".js": "text/javascript" };
const server = createServer(async (req, res) => {
    try {
        const path = join(labDir, decodeURIComponent(new URL(req.url, "http://x").pathname).replace(/^\/$/, "/index.html"));
        res.writeHead(200, { "content-type": types[extname(path)] ?? "application/octet-stream" });
        res.end(await readFile(path));
    } catch { res.writeHead(404); res.end(); }
}).listen(0);
const port = server.address().port;

const browser = await chromium.launch({ args: ["--use-angle=swiftshader", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist"] });
const page = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
page.on("console", (m) => { if (m.type() === "error") console.error("page:", m.text()); });
await page.route("https://cdn.jsdelivr.net/npm/three@0.170.0/**", async (route) => {
    const rel = new URL(route.request().url()).pathname.replace("/npm/three@0.170.0/", "");
    route.fulfill({ body: await readFile(join(threeDir, rel)), contentType: "text/javascript" });
});
await page.route("https://fonts.googleapis.com/**", (route) => route.abort());
await page.goto(`http://localhost:${port}/index.html?capture`);
await page.waitForFunction(() => window.lab, null, { timeout: 120000 });

const shot = async (name, settings, time = 1, yaw = 0.5, clip = null) => {
    await page.evaluate(([s, t, y]) => { window.lab.set(s); window.lab.still(t, y); }, [settings, time, yaw]);
    await page.screenshot({ path: join(outDir, name), clip: clip ?? undefined });
    const stats = await page.evaluate(() => window.lab.stats());
    console.log(name, JSON.stringify(stats));
};
const stage = { x: 0, y: 0, width: 1280, height: 1000 };
await shot("matrix.png", { view: "matrix", structure: 3, surface: 2, open: 0.35, spin: 0, turntable: false, bloom: true }, 1, 0, stage);
await shot("unit-hero.png", { view: "unit", structure: 3, surface: 3, open: 0.6, spin: 0.6, turntable: true }, 1, 0.2, stage);
await shot("unit-closed-bevel.png", { view: "unit", structure: 3, surface: 2, open: 0, spin: 0.6 }, 1, 0.2, stage);
await shot("unit-stretched.png", { view: "unit", structure: 2, surface: 2, open: 0, stretch: 1 }, 1, 1.2, stage);
await shot("underside.png", { view: "unit", structure: 3, surface: 2, open: 0, stretch: 0, underside: true }, 1, 0.4, stage);
await shot("swarm.png", { view: "swarm", underside: false, open: 0.35 }, 1, 0, stage);
await shot("lab-panel.png", { view: "unit", structure: 3, surface: 3, open: 0.35 }, 1, 0.55);
const triangles = await page.evaluate(() => ({ table: window.lab.triangleTable, presets: window.lab.presets }));
await writeFile(join(outDir, "triangles.json"), JSON.stringify(triangles, null, 1));

if (video) {
    // Turntable through the travel/contact cycle, one exact frame per 1/30 s.
    await page.setViewportSize({ width: 1280, height: 720 });
    await page.evaluate(() => window.lab.set({ view: "unit", structure: 3, surface: 3, cycle: true, spin: 0.6, underside: false }));
    const frames = join(outDir, "frames");
    await mkdir(frames, { recursive: true });
    const count = Number(process.env.FRAMES ?? 240);
    for (let i = 0; i < count; ++i) {
        const t = i / 30;
        await page.evaluate(([time, yaw]) => window.lab.still(time, yaw), [t, 0.4 + t * 0.35]);
        await page.screenshot({ path: join(frames, `f${String(i).padStart(4, "0")}.png`), clip: { x: 0, y: 0, width: 960, height: 720 } });
    }
    await new Promise((resolve, reject) => spawn("ffmpeg", ["-y", "-loglevel", "error", "-framerate", "30", "-i", join(frames, "f%04d.png"),
        "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "20", join(outDir, "turntable.mp4")], { stdio: "inherit" })
        .on("exit", (code) => code ? reject(new Error("ffmpeg " + code)) : resolve()));
}
await browser.close();
server.close();
