#!/usr/bin/env python3
"""
  ./tte.py [WORKDIR] [GAP]      defaults: workdir, the campaign time limit

TTE: elapsed seconds of the first monitor snapshot whose bug _T (triggered) counter is non-zero.
Running: reads workdir/cache/<fuzzer>/<target>/<program>/<cid>/monitor/.
Finished: reads workdir/ar/<fuzzer>/<target>/<program>/<cid>/ball.tar.
"""
import bisect, csv, io, os, re, statistics, sys, tarfile, time


def snapshots(campaign):
    ball = os.path.join(campaign, "ball.tar")
    if os.path.isfile(ball):
        with tarfile.open(ball) as t:
            members = [m for m in t.getmembers()
                       if m.isfile() and re.fullmatch(r"\./monitor/\d+", m.name)]
            for m in sorted(members, key=lambda m: int(m.name.rsplit("/", 1)[1])):
                yield int(m.name.rsplit("/", 1)[1]), \
                      t.extractfile(m).read().decode("utf-8", "replace")
    else:
        d = os.path.join(campaign, "monitor")
        for name in sorted(os.listdir(d), key=int) if os.path.isdir(d) else []:
            yield int(name), open(os.path.join(d, name), errors="replace").read()


def stamp(campaign):
    ball = os.path.join(campaign, "ball.tar")
    if os.path.isfile(ball):
        return os.path.getmtime(ball)
    d = os.path.join(campaign, "monitor")
    return os.path.getmtime(d) if os.path.isdir(d) else 0.0


def tte(campaign):
    for sec, text in snapshots(campaign):
        rows = list(csv.reader(io.StringIO(text)))
        if len(rows) < 2:
            continue
        for col, val in zip(rows[0], rows[1]):
            if col.endswith("_T") and val.strip() not in ("", "0"):
                return sec
    return None


def limit(workdir):
    try:
        rc = open(os.path.join(workdir, "captainrc")).read()
        v = re.search(r"^TIMEOUT=(\d+)([smhd]?)$", rc, re.M)
        return int(v.group(1)) * {"": 1, "s": 1, "m": 60, "h": 3600, "d": 86400}[v.group(2)]
    except Exception:
        return None


def has_data(campaign):
    return os.path.isfile(os.path.join(campaign, "ball.tar")) or \
           os.path.isdir(os.path.join(campaign, "monitor"))


def collect(root, results, running):
    if not os.path.isdir(root):
        return
    for fuzzer in sorted(os.listdir(root)):
        for target in sorted(os.listdir(os.path.join(root, fuzzer))):
            m = re.fullmatch(r"(.+)_([a-z]+\d+)_(base|locus)", target)
            if not m:
                continue
            _, bug, variant = m.groups()
            tdir = os.path.join(root, fuzzer, target)
            for program in sorted(os.listdir(tdir)):
                pdir = os.path.join(tdir, program)
                key = (fuzzer, bug.upper(), program)
                for cid in sorted(os.listdir(pdir), key=int):
                    campaign = os.path.join(pdir, cid)
                    if not has_data(campaign):
                        continue
                    bucket = running if running is not None else results
                    bucket.setdefault(key, {}).setdefault(variant, []).append(
                        (tte(campaign), stamp(campaign)))


def split(results, gap):
    stamps = sorted(s for v in results.values() for runs in v.values() for _, s in runs)
    if not stamps:
        return []
    bounds = [stamps[i] for i in range(1, len(stamps)) if stamps[i] - stamps[i - 1] > gap]
    runs = [{} for _ in range(len(bounds) + 1)]
    for key, variants in results.items():
        for variant, entries in variants.items():
            for t, s in entries:
                runs[bisect.bisect_right(bounds, s)].setdefault(key, {}) \
                    .setdefault(variant, []).append(t)
    labels = []
    for i, first in enumerate([stamps[0]] + bounds):
        when = time.strftime("%m-%d %H:%M", time.localtime(first))
        labels.append(f"run {i + 1}/{len(runs)}, {when}" if len(runs) > 1 else "")
    return list(zip(labels, runs))


def report(title, results, cap, live):
    for (fuzzer, bug, program), variants in sorted(results.items()):
        print(f"\n{bug}  {fuzzer}  {program}  [{title}]"
              + ("" if live or not cap else f"  (limit {cap}s)"))
        print(f"  {'variant':<8} {'trials':>6} {'found':>7} {'median':>9} {'mean':>9}   TTEs")
        median, mean = {}, {}
        for variant in ("base", "locus"):
            runs = variants.get(variant)
            if not runs:
                continue
            hits = [t for t in runs if t is not None]
            med = statistics.median(hits) if hits else None
            avg = statistics.mean(hits) if hits else None
            median[variant], mean[variant] = med, avg
            miss = "..." if live else "T.O."
            shown = ", ".join(str(t) if t is not None else miss for t in runs)
            print(f"  {variant:<8} {len(runs):>6} {len(hits)}/{len(runs):<5} "
                  f"{(f'{med:g}s' if med else '-'):>9} "
                  f"{(f'{avg:.0f}s' if avg else '-'):>9}   {shown}")
        if median.get("base") and median.get("locus"):
            print(f"  speedup (median base / median locus): "
                  f"{median['base'] / median['locus']:.2f}x")
        if mean.get("base") and mean.get("locus"):
            print(f"  speedup (mean base / mean locus):     "
                  f"{mean['base'] / mean['locus']:.2f}x")


def main():
    workdir = sys.argv[1] if len(sys.argv) > 1 else "workdir"
    cap = limit(workdir)
    gap = int(sys.argv[2]) if len(sys.argv) > 2 else (cap or 3600)
    results, running = {}, {}
    collect(os.path.join(workdir, "ar"), results, None)
    collect(os.path.join(workdir, "cache"), None, running)
    if not results and not running:
        raise SystemExit(f"no campaigns under {workdir}")

    for name, bucket, live in (("finished", results, False), ("running", running, True)):
        runs = split(bucket, gap)
        for label, one in runs:
            report(f"{name}, {label}" if label else name, one, cap, live)
    print()


if __name__ == "__main__":
    main()
