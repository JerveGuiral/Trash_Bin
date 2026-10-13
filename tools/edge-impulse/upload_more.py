"""Upload extra Kaggle images from <folder> as <label>, skipping the ones
upload_kaggle.py already uploaded (it is re-run in --dry-run style to find them).

Usage: EI_API_KEY=ei_... python upload_more.py <archive.zip> <folder> <label> <count> [--dry-run]
"""
import os
import random
import sys
import zipfile

sys.argv_backup = list(sys.argv)
ZIP_PATH, SRC_FOLDER, LABEL, COUNT = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
DRY_RUN = "--dry-run" in sys.argv

# Reuse the exact selection logic of the first upload to know which files were used
sys.argv = [sys.argv[0], ZIP_PATH, "--dry-run"]
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import upload_kaggle as first  # noqa: E402

zf = zipfile.ZipFile(ZIP_PATH)
used = {member for _, _, member, _ in first.pick(zf)}
all_bio = sorted(n for n in zf.namelist()
                 if n.startswith(first.ROOT + SRC_FOLDER + "/") and n.lower().endswith((".jpg", ".jpeg", ".png")))
fresh = [n for n in all_bio if n not in used]
print(f"{SRC_FOLDER}: {len(all_bio)} total, {len(all_bio) - len(fresh)} already uploaded, {len(fresh)} unused")

chosen = random.Random(7).sample(fresh, COUNT)
n_test = int(COUNT * first.TEST_FRACTION)
jobs = []
for i, member in enumerate(chosen):
    category = "testing" if i < n_test else "training"
    jobs.append((category, LABEL, member,
                 f"{LABEL}.kaggle-{SRC_FOLDER}-extra-{os.path.basename(member)}"))
print(f"to upload: {len(jobs) - n_test} training, {n_test} testing")
if DRY_RUN:
    sys.exit(0)

first.API_KEY = os.environ["EI_API_KEY"]
done, failed = first.upload(zf, jobs)
print(f"DONE: uploaded {done}, failed {failed}")
