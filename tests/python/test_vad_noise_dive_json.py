import json
import tempfile
import numpy as np


def test_e_set_json_serialization_deserialization():
    # Construct synthetic clips object with numpy arrays similar to e_build.py
    clips = [
        {
            "name": "clean_00",
            "cond": "clean",
            "kind": "speech",
            "spans": [[0.1, 1.5], [2.0, 3.2]],
            "z": {
                "redux": np.array([0.5, 1.2, -0.3], dtype=np.float32),
                "ultra": np.array([0.8, 2.1, -0.1], dtype=np.float32),
            },
            "sil": np.array([0.1, 0.9, 0.2], dtype=np.float32),
            "e10": np.array([-20.5, -15.2, -30.0], dtype=np.float64),
        }
    ]

    with tempfile.NamedTemporaryFile(mode="w+", suffix=".json", delete=False) as tmp:
        # Test serialization format used in e_build.py
        json.dump(
            clips,
            tmp,
            default=lambda o: o.tolist() if hasattr(o, "tolist") else o,
        )
        tmp_path = tmp.name

    # Test deserialization format used in e_eval.py and fig_energy.py
    loaded_clips = json.load(open(tmp_path, "r"))
    for c in loaded_clips:
        c["e10"] = np.array(c["e10"], dtype=np.float64)
        if "sil" in c:
            c["sil"] = np.array(c["sil"], dtype=np.float32)
        if "z" in c:
            for k in c["z"]:
                c["z"][k] = np.array(c["z"][k], dtype=np.float32)

    assert len(loaded_clips) == 1
    c = loaded_clips[0]
    assert c["name"] == "clean_00"
    assert isinstance(c["e10"], np.ndarray)
    assert isinstance(c["sil"], np.ndarray)
    assert isinstance(c["z"]["redux"], np.ndarray)
    np.testing.assert_allclose(c["e10"], clips[0]["e10"])
    np.testing.assert_allclose(c["sil"], clips[0]["sil"])
    np.testing.assert_allclose(c["z"]["redux"], clips[0]["z"]["redux"])
