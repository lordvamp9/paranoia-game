// Generates build/icon.ico — a 256x256 eye on black, PNG-in-ICO.
// No dependencies: raw RGBA -> PNG (zlib) -> ICO container.
const zlib = require('zlib');
const fs = require('fs');
const path = require('path');

const S = 256;
const px = Buffer.alloc(S * S * 4);

function set(x, y, r, g, b, a = 255) {
  if (x < 0 || y < 0 || x >= S || y >= S) return;
  const i = (y * S + x) * 4;
  px[i] = r; px[i + 1] = g; px[i + 2] = b; px[i + 3] = a;
}

// background: black
for (let y = 0; y < S; y++) for (let x = 0; x < S; x++) set(x, y, 4, 4, 6);

const cx = 128, cy = 128;
for (let y = 0; y < S; y++) {
  for (let x = 0; x < S; x++) {
    const dx = x - cx, dy = y - cy;
    const sclera = (dx * dx) / (108 * 108) + (dy * dy) / (52 * 52);
    if (sclera < 1) {
      const v = 175 - sclera * 60;
      set(x, y, v, v, v + 6);
      const d2 = dx * dx + dy * dy;
      if (d2 < 40 * 40) { // iris
        const t = Math.sqrt(d2) / 40;
        const v2 = 18 + t * 50;
        set(x, y, v2, v2, v2 + 8);
      }
      if (d2 < 16 * 16) set(x, y, 120, 16, 14); // red pupil
      if ((dx + 14) * (dx + 14) + (dy + 14) * (dy + 14) < 36) set(x, y, 230, 230, 235); // glint
    } else if (sclera < 1.12) {
      set(x, y, 60, 58, 56); // rim
    }
  }
}

// ---- PNG encode
const crcTable = [];
for (let n = 0; n < 256; n++) {
  let c = n;
  for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
  crcTable[n] = c >>> 0;
}
function crc32(buf) {
  let c = 0xffffffff;
  for (const b of buf) c = crcTable[(c ^ b) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}
function chunk(type, data) {
  const len = Buffer.alloc(4);
  len.writeUInt32BE(data.length);
  const td = Buffer.concat([Buffer.from(type), data]);
  const crc = Buffer.alloc(4);
  crc.writeUInt32BE(crc32(td));
  return Buffer.concat([len, td, crc]);
}
const ihdr = Buffer.alloc(13);
ihdr.writeUInt32BE(S, 0); ihdr.writeUInt32BE(S, 4);
ihdr[8] = 8; ihdr[9] = 6; // 8-bit RGBA
const raw = Buffer.alloc(S * (S * 4 + 1));
for (let y = 0; y < S; y++) {
  raw[y * (S * 4 + 1)] = 0;
  px.copy(raw, y * (S * 4 + 1) + 1, y * S * 4, (y + 1) * S * 4);
}
const png = Buffer.concat([
  Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
  chunk('IHDR', ihdr),
  chunk('IDAT', zlib.deflateSync(raw, { level: 9 })),
  chunk('IEND', Buffer.alloc(0))
]);

// ---- ICO container (PNG payload allowed for 256x256)
const header = Buffer.from([0, 0, 1, 0, 1, 0]);
const entry = Buffer.alloc(16);
entry[0] = 0; entry[1] = 0; // 256x256
entry[4] = 1; // planes
entry[6] = 32; // bpp
entry.writeUInt32LE(png.length, 8);
entry.writeUInt32LE(22, 12);
fs.mkdirSync(path.join(__dirname, '..', 'build'), { recursive: true });
fs.writeFileSync(path.join(__dirname, '..', 'build', 'icon.ico'), Buffer.concat([header, entry, png]));
console.log('icon.ico written,', png.length, 'bytes png');
