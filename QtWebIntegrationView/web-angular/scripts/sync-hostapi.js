const fs = require("fs");
const path = require("path");

const repoRoot = path.resolve(__dirname, "../..");
const srcDir = path.join(repoRoot, "build", "generated", "hostapi", "angular");
const destDir = path.join(__dirname, "..", "src", "generated");

if (!fs.existsSync(srcDir)) {
  console.error("HostApi generator output not found at", srcDir);
  process.exit(1);
}

fs.mkdirSync(destDir, { recursive: true });

for (const entry of fs.readdirSync(srcDir)) {
  if (!entry.endsWith(".ts")) {
    continue;
  }
  const srcPath = path.join(srcDir, entry);
  const destPath = path.join(destDir, entry);
  fs.copyFileSync(srcPath, destPath);
}

console.log("HostApi Angular services synced to", destDir);
