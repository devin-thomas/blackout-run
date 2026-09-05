from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / "site"


class SiteReleaseTests(unittest.TestCase):
    def test_local_links_and_images_exist(self) -> None:
        html = (SITE / "index.html").read_text(encoding="utf-8")
        references = re.findall(r'(?:href|src)="([^"]+)"', html)
        for reference in references:
            if reference.startswith(("#", "http")):
                continue
            with self.subTest(reference=reference):
                self.assertTrue((SITE / reference).is_file())

    def test_all_images_have_alt_text_and_native_dimensions(self) -> None:
        html = (SITE / "index.html").read_text(encoding="utf-8")
        images = re.findall(r"<img\s+[^>]+>", html)
        self.assertGreaterEqual(len(images), 5)
        for image in images:
            self.assertRegex(image, r'alt="[^"]+"')
            self.assertIn('width="240"', image)
            self.assertIn('height="160"', image)

    def test_screens_use_nearest_neighbor_scaling(self) -> None:
        css = (SITE / "styles.css").read_text(encoding="utf-8")
        self.assertIn("width: 960px", css)
        self.assertIn("image-rendering: pixelated", css)


if __name__ == "__main__":
    unittest.main()
