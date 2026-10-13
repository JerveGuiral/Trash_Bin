"""Upload a balanced plastic / biodegradable subset of the Kaggle
"Garbage Classification (12 classes)" dataset to Edge Impulse.

Usage: EI_API_KEY=ei_... python upload_kaggle.py <archive.zip> [--dry-run]
Reads images straight from the zip (no extraction). 80% training / 20% testing.
"""
import os
import random
import sys
import time
import zipfile

import requests

ZIP_PATH = sys.argv[1]
DRY_RUN = "--dry-run" in sys.argv
API_KEY = os.environ.get("EI_API_KEY", "")
ROOT = "garbage_classification/"

# label -> {source folder: number of images}
PLAN = {
    "plastic": {"plastic": 600},
    "biodegradable": {"biological": 350, "paper": 125, "cardboard": 125},
}
TEST_FRACTION = 0.2
BATCH = 20
URL = "https://ingestion.edgeimpulse.com/api/{category}/files"


def pick(zf):
    names = zf.namelist()
    rng = random.Random(42)
    jobs = []  # (category, label, zip member, upload name)
    for label, sources in PLAN.items():
        chosen = []
        for folder, count in sources.items():
            files = sorted(n for n in names
                           if n.startswith(ROOT + folder + "/") and n.lower().endswith((".jpg", ".jpeg", ".png")))
            if len(files) < count:
                raise SystemExit(f"{folder}: only {len(files)} images, need {count}")
            chosen += rng.sample(files, count)
        rng.shuffle(chosen)
        n_test = int(len(chosen) * TEST_FRACTION)
        for i, member in enumerate(chosen):
            category = "testing" if i < n_test else "training"
            base = os.path.basename(member)
            folder = member[len(ROOT):].split("/")[0]
            jobs.append((category, label, member, f"{label}.kaggle-{folder}-{base}"))
    return jobs


def upload(zf, jobs):
    groups = {}
    for job in jobs:
        groups.setdefault((job[0], job[1]), []).append(job)
    done = failed = 0
    for (category, label), items in groups.items():
        for start in range(0, len(items), BATCH):
            batch = items[start:start + BATCH]
            files = []
            for _, _, member, name in batch:
                ctype = "image/png" if name.lower().endswith(".png") else "image/jpeg"
                files.append(("data", (name, zf.read(member), ctype)))
            for attempt in range(3):
                try:
                    r = requests.post(URL.format(category=category), files=files, timeout=120,
                                      headers={"x-api-key": API_KEY, "x-label": label})
                    if r.status_code == 200:
                        done += len(batch)
                        break
                    print(f"  HTTP {r.status_code}: {r.text[:200]}")
                    if r.status_code in (401, 403):
                        raise SystemExit("API key rejected")
                except requests.RequestException as e:
                    print(f"  network error: {e}")
                time.sleep(3)
            else:
                failed += len(batch)
            print(f"{category:8s} {label:14s} {min(start + BATCH, len(items))}/{len(items)}  (total ok {done}, failed {failed})",
                  flush=True)
    return done, failed


def main():
    zf = zipfile.ZipFile(ZIP_PATH)
    jobs = pick(zf)
    summary = {}
    for category, label, _, _ in jobs:
        summary[(label, category)] = summary.get((label, category), 0) + 1
    for (label, category), n in sorted(summary.items()):
        print(f"{label:14s} {category:8s} {n}")
    if DRY_RUN:
        return
    if not API_KEY.startswith("ei_"):
        raise SystemExit("Set EI_API_KEY")
    done, failed = upload(zf, jobs)
    print(f"DONE: uploaded {done}, failed {failed}")


if __name__ == "__main__":
    main()
