"""Upload camera photos (<label>.<timestamp>.jpg) from a folder to Edge Impulse, 80% training / 20% testing.

Usage: EI_API_KEY=ei_... python upload_files.py <folder> <label>
"""
import glob
import os
import random
import sys
import time

import requests

FOLDER, LABEL = sys.argv[1], sys.argv[2]
API_KEY = os.environ["EI_API_KEY"]
URL = "https://ingestion.edgeimpulse.com/api/{category}/files"
BATCH = 20

files = sorted(glob.glob(os.path.join(FOLDER, f"{LABEL}.[0-9]*.jpg")))
random.Random(42).shuffle(files)
n_test = int(len(files) * 0.2)
split = {"testing": files[:n_test], "training": files[n_test:]}

done = failed = 0
for category, items in split.items():
    for start in range(0, len(items), BATCH):
        batch = items[start:start + BATCH]
        payload = [("data", (os.path.basename(p), open(p, "rb").read(), "image/jpeg")) for p in batch]
        for attempt in range(3):
            try:
                r = requests.post(URL.format(category=category), files=payload, timeout=120,
                                  headers={"x-api-key": API_KEY, "x-label": LABEL})
                if r.status_code == 200:
                    done += len(batch)
                    break
                print(f"  HTTP {r.status_code}: {r.text[:200]}")
            except requests.RequestException as e:
                print(f"  network error: {e}")
            time.sleep(3)
        else:
            failed += len(batch)
    print(f"{category}: {len(items)} files")
print(f"DONE: uploaded {done}, failed {failed}")
