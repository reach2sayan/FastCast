#!/usr/bin/env python3
"""Render the README benchmark plots from a google-benchmark JSON dump.

Usage:
    ./measure --benchmark_filter='-.*threads:' \
              --benchmark_repetitions=5 \
              --benchmark_report_aggregates_only=true \
              --benchmark_format=json --benchmark_out=results.json
    python3 plot_results.py results.json plots/
"""
import json
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

FAST = "#00599C"   # fast_cast
DYN = "#C0392B"    # dynamic_cast
STAT = "#7F8C8D"   # static_cast


def load(path):
    data = json.load(open(path))["benchmarks"]
    out = {}
    for b in data:
        name = b["name"]
        if not name.endswith("_mean"):
            continue
        key = name[: -len("_mean")]
        out[key] = (b["real_time"], b.get("time_unit", "ns"))
    return out


def bars(ax, labels, fast, dyn, unit, title, log=False, extra=None):
    import numpy as np

    x = np.arange(len(labels))
    w = 0.38
    ax.bar(x - w / 2, fast, w, label="fast_cast", color=FAST)
    ax.bar(x + w / 2, dyn, w, label="dynamic_cast", color=DYN)
    if extra is not None:
        evals, elabel = extra
        ax.bar(x + w / 2, evals, w, color=STAT, alpha=0)  # spacing only
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.set_ylabel(f"latency ({unit}) — lower is better")
    ax.set_title(title)
    if log:
        ax.set_yscale("log")
    ax.legend()
    ax.grid(axis="y", ls=":", alpha=0.5)
    for i, (f, d) in enumerate(zip(fast, dyn)):
        ax.annotate(f"{f:.3g}", (i - w / 2, f), ha="center", va="bottom", fontsize=8)
        ax.annotate(f"{d:.3g}", (i + w / 2, d), ha="center", va="bottom", fontsize=8)


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "results.json"
    outdir = Path(sys.argv[2] if len(sys.argv) > 2 else "plots")
    outdir.mkdir(parents=True, exist_ok=True)
    r = load(src)

    def t(k):
        return r[k][0]

    # 1) Cold (cache-miss) vs hot (cache-hit) -- the honest comparison.
    fig, ax = plt.subplots(figsize=(7, 4.5))
    bars(
        ax,
        ["Cold\n(cache miss)", "Hot\n(cache hit)"],
        [t("BM_FastDynamicCast_Cold"), t("BM_FastDynamicCast_Hot")],
        [t("BM_DynamicCast_Cold"), t("BM_DynamicCast_Hot")],
        "ns",
        "Cold vs hot: ComplexA* -> ComplexB* (log scale)",
        log=True,
    )
    fig.tight_layout()
    fig.savefig(outdir / "cold_vs_hot.png", dpi=130)
    plt.close(fig)

    # 2) Per-call latency on a reused object (all hot for fast_cast).
    fig, ax = plt.subplots(figsize=(7, 4.5))
    bars(
        ax,
        ["Ptr success", "Ptr failure", "Ref reused"],
        [
            t("BM_FastDynamicCast_Ptr_Success"),
            t("BM_FastDynamicCast_Ptr_Failure"),
            t("BM_FastDynamicCast_Reused"),
        ],
        [
            t("BM_DynamicCast_Ptr_Success"),
            t("BM_DynamicCast_Ptr_Failure"),
            t("BM_DynamicCast_Reused"),
        ],
        "ns",
        "Repeated cast of the same object (hot cache)",
    )
    fig.tight_layout()
    fig.savefig(outdir / "per_call.png", dpi=130)
    plt.close(fig)

    # 3) Throughput over 2,000,000 iterations (ms).
    fig, ax = plt.subplots(figsize=(7, 4.5))
    bars(
        ax,
        ["Simple\nhierarchy", "Complex\nhierarchy"],
        [t("BM_FastDynamicCast_Simple/2000000"), t("BM_FastDynamicCast_Complex/2000000")],
        [t("BM_DynamicCast_Simple/2000000"), t("BM_DynamicCast_Complex/2000000")],
        "ms",
        "2,000,000 reference casts (lower is better)",
    )
    fig.tight_layout()
    fig.savefig(outdir / "throughput.png", dpi=130)
    plt.close(fig)

    print("wrote:", *[str(p) for p in sorted(outdir.glob("*.png"))])


if __name__ == "__main__":
    main()
