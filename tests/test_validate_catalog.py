import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VALIDATOR = ROOT / "scripts" / "validate_catalog.py"


class CatalogValidatorTests(unittest.TestCase):
    def run_validator(self, payload):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "catalog.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            return subprocess.run(
                [sys.executable, str(VALIDATOR), str(path)],
                text=True,
                capture_output=True,
                check=False,
            )

    def valid_catalog(self):
        return {
            "version": 1,
            "title": "Test",
            "items": [{
                "id": "demo",
                "title": "Demo",
                "source": {
                    "type": "magnet",
                    "uri": "magnet:?xt=urn:btih:0123456789abcdef"
                }
            }]
        }

    def test_example_catalog_passes(self):
        result = subprocess.run(
            [sys.executable, str(VALIDATOR), str(ROOT / "catalog" / "catalog.example.json")],
            text=True, capture_output=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_duplicate_ids_fail(self):
        payload = self.valid_catalog()
        payload["items"].append(dict(payload["items"][0]))
        result = self.run_validator(payload)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("duplicate id", result.stderr)

    def test_invalid_magnet_scheme_fails(self):
        payload = self.valid_catalog()
        payload["items"][0]["source"]["uri"] = "https://example.invalid/demo"
        result = self.run_validator(payload)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("magnet", result.stderr)

    def test_http_torrent_url_passes(self):
        payload = self.valid_catalog()
        payload["items"][0]["source"] = {
            "type": "torrent_url",
            "uri": "https://example.invalid/demo.torrent"
        }
        result = self.run_validator(payload)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_unknown_source_type_fails(self):
        payload = self.valid_catalog()
        payload["items"][0]["source"]["type"] = "shell"
        result = self.run_validator(payload)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("source.type", result.stderr)


if __name__ == "__main__":
    unittest.main()
