import json
import tempfile
from unittest.mock import patch, MagicMock
import pytest

import sys
from pathlib import Path

# Add scripts directory to sys.path so lib can be imported
trim_regression_scripts = Path(__file__).parent.parent / "scripts" / "vad_bench" / "trim_regression" / "scripts"
sys.path.insert(0, str(trim_regression_scripts))


def test_file_errors_json_caching():
    mock_manifest_data = [
        {
            "kind": "talk",
            "ref": "hello world",
            "dur": 1.0,
            "name": "test_file",
        }
    ]

    import common
    with patch.object(common, "manifest", return_value=mock_manifest_data):
        import lib

        with tempfile.TemporaryDirectory() as tmpdir:
            det = "redux"
            name = "test_file"
            vn = "T0.3"

            with patch.object(lib, "W", tmpdir), \
                 patch.object(lib, "segs_for", return_value=[(0.0, 1.0)]), \
                 patch.object(lib, "hyp", return_value=[("hello", 0.0, 0.5, 0, 0.9), ("world", 0.5, 1.0, 0, 0.9)]):

                # Update MAN inside lib for test
                lib.MAN = {m["name"]: m for m in mock_manifest_data}

                # 1. Compute file errors and write json cache
                res1 = lib.file_errors(det, name, vn)

                cache_file = Path(tmpdir) / "cache" / "err" / det / vn / f"{name}.json"
                assert cache_file.exists()

                # Inspect cache file on disk
                with open(cache_file, "r") as fp:
                    cached_raw = json.load(fp)

                assert "units" in cached_raw
                assert "events" in cached_raw
                assert "sdi" in cached_raw

                # 2. Call file_errors again to load from JSON cache
                res2 = lib.file_errors(det, name, vn)

                assert res1 == res2
                assert isinstance(res2["units"], dict)
                # Ensure tuple keys in units are properly deserialized
                for k in res2["units"].keys():
                    assert isinstance(k, tuple)
                for ev in res2["events"]:
                    assert isinstance(ev, tuple)
