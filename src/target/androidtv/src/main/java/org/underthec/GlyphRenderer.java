package org.underthec;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.util.SparseIntArray;

import java.util.ArrayList;
import java.util.List;

final class GlyphRenderer {
  private static final int PAGE = 1024;

  private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Paint fill = new Paint();
  private final Paint glyph = new Paint();
  private final Rect src = new Rect();
  private final Rect dst = new Rect();
  private final char[] one = new char[2];
  private final List<Bitmap> pages = new ArrayList<>();
  private final SparseIntArray slotOf = new SparseIntArray();

  private int slotCount;
  private int slotsPerRow;
  private int slotsPerPage;
  private int cellW = 1;
  private int cellH = 1;
  private float baseline;

  GlyphRenderer() {
    text.setTypeface(Typeface.MONOSPACE);
    text.setColor(0xffffffff);
  }

  void reset(float textPx, int newCellW, int newCellH, float newBaseline) {
    for (Bitmap b : pages) b.recycle();
    pages.clear();
    slotOf.clear();
    slotCount = 0;
    text.setTextSize(textPx);
    cellW = newCellW;
    cellH = newCellH;
    baseline = newBaseline;
    slotsPerRow = Math.max(1, PAGE / (2 * cellW));
    slotsPerPage = slotsPerRow * Math.max(1, PAGE / cellH);
    for (int cp = 33; cp < 127; cp++) slotFor(cp);
  }

  private int slotFor(int cp) {
    int idx = slotOf.get(cp, -1);
    if (idx >= 0) return idx;
    idx = slotCount++;
    slotOf.put(cp, idx);
    int page = idx / slotsPerPage;
    while (pages.size() <= page) pages.add(Bitmap.createBitmap(PAGE, PAGE, Bitmap.Config.ALPHA_8));
    int inPage = idx % slotsPerPage;
    int x = inPage % slotsPerRow * 2 * cellW;
    int y = inPage / slotsPerRow * cellH;
    Canvas c = new Canvas(pages.get(page));
    c.save();
    c.clipRect(x, y, x + 2 * cellW, y + cellH);
    int n = Character.toChars(cp, one, 0);
    c.drawText(one, 0, n, x, y + baseline, text);
    c.restore();
    return idx;
  }

  void draw(Canvas canvas, int[] cells, int cols, int rows, int offX, int offY) {
    canvas.save();
    canvas.translate(offX, offY);
    for (int r = 0; r < rows; r++) {
      drawBackgrounds(canvas, cells, cols, r);
      drawGlyphs(canvas, cells, cols, r);
    }
    canvas.restore();
  }

  private void drawBackgrounds(Canvas canvas, int[] cells, int cols, int r) {
    int base = r * cols;
    int c = 0;
    while (c < cols) {
      int key = cells[base + c] >> 5 & 0x1f;
      int start = c;
      while (c < cols && (cells[base + c] >> 5 & 0x1f) == key) c++;
      int bg = key & 0xf;
      if (bg == 0) continue;
      fill.setColor(Palette.argb(bg, (key >> 4 & 1) != 0));
      canvas.drawRect(start * cellW, r * cellH, c * cellW, (r + 1) * cellH, fill);
    }
  }

  private void drawGlyphs(Canvas canvas, int[] cells, int cols, int r) {
    int base = r * cols;
    for (int c = 0; c < cols; c++) {
      int v = cells[base + c];
      int cp = v >>> 10;
      if (cp == 0 || cp == ' ') continue;
      glyph.setColor(Palette.argb(v >> 1 & 0xf, (v & 1) != 0));
      int idx = slotFor(cp);
      int inPage = idx % slotsPerPage;
      int sx = inPage % slotsPerRow * 2 * cellW;
      int sy = inPage / slotsPerRow * cellH;
      int span = c + 1 < cols && cells[base + c + 1] >>> 10 == 0 ? 2 : 1;
      src.set(sx, sy, sx + span * cellW, sy + cellH);
      dst.set(c * cellW, r * cellH, (c + span) * cellW, (r + 1) * cellH);
      canvas.drawBitmap(pages.get(idx / slotsPerPage), src, dst, glyph);
    }
  }
}
