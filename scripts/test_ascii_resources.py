#!/usr/bin/env python3

import pathlib
import tempfile
import unittest

import ascii_resources


VALID = """pascii 1
namespace demo_assets
output demo/src/demo_assets.h

sprite kLowResolution
size 3 2
padding 1 2
anchor 0.25 0.75
blend alpha
pixels
|.=+|
| _ |
end

sprite kHighResolution
size 6 1
padding 0 0
anchor 0.5 0.5
blend opaque
pixels
|::==##|
end
"""


class AsciiResourcesTest(unittest.TestCase):
    def parse(self, text):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        path = pathlib.Path(directory.name) / "sprites.pascii"
        path.write_text(text, encoding="utf-8")
        return path, ascii_resources.parse_resource(path)

    def test_round_trip_is_lossless(self):
        _, resource = self.parse(VALID)
        self.assertEqual(VALID, ascii_resources.serialize_resource(resource))
        self.assertEqual((3, 2),
                         (resource.sprites[0].width, resource.sprites[0].height))
        self.assertEqual((6, 1),
                         (resource.sprites[1].width, resource.sprites[1].height))

    def test_generator_merges_horizontal_runs(self):
        path, resource = self.parse(VALID)
        generated = ascii_resources.generate_header(path, resource)
        self.assertIn("{2, 0, 2, kAsciiBase}", generated)
        self.assertIn("{4, 0, 2, kAsciiHotGlow}", generated)

    def test_rejects_unknown_ink(self):
        with self.assertRaisesRegex(ascii_resources.ResourceError,
                                    "unknown ink"):
            self.parse(VALID.replace(".=+", ".?+"))

    def test_rejects_wrong_row_width(self):
        with self.assertRaisesRegex(ascii_resources.ResourceError,
                                    "row width"):
            self.parse(VALID.replace(".=+", ".="))


if __name__ == "__main__":
    unittest.main()
