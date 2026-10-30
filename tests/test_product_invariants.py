import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DAEMON = (ROOT / "vendor/ps4torrent/main.cpp").read_text(encoding="utf-8")
SWARM = (ROOT / "vendor/ps4torrent/swarm.cpp").read_text(encoding="utf-8")
UI = (ROOT / "vendor/ps4torrent/pkg/src/ui.cpp").read_text(encoding="utf-8")


class ProductInvariantTests(unittest.TestCase):
    def test_download_only_listener_is_disabled_by_default(self):
        self.assertIn('#define LISTEN_PORT 0', DAEMON)
        self.assertIn('g_listenPort = 0;', DAEMON)
        self.assertIn('incoming_stop();', DAEMON)

    def test_upload_requests_are_choked(self):
        self.assertIn('id == 6 && len >= 13', SWARM)
        self.assertIn('static const unsigned char choke[5] = {0, 0, 0, 1, 0};', SWARM)

    def test_internal_download_root_is_vaynax_download(self):
        self.assertIn('#define INTERNAL_ROOT "/data/Download"', DAEMON)
        for folder in ('Games', 'Movies', 'Series', 'Other'):
            self.assertIn('"' + folder + '"', DAEMON)

    def test_ui_does_not_offer_a_peer_listening_port(self):
        self.assertNotIn('id="lp"', DAEMON)
        self.assertNotIn("$('lp')", DAEMON)

    def test_ui_branding_and_flag_progress_are_present(self):
        self.assertIn('VAYNAX TORRENT v', UI)
        self.assertIn('flagGreen', UI)
        self.assertIn('flagWhite', UI)
        self.assertIn('flagRed', UI)


if __name__ == "__main__":
    unittest.main()
