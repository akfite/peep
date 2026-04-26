#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

usage() {
    cat <<EOF
Usage: $(basename "$0") [--build] [build-dir]

Runs all numbered peep examples in order.

Options:
  --build    Configure and build the examples before running them.

Arguments:
  build-dir  CMake build directory. Defaults to ./build.
EOF
}

build_first=0
build_dir="${repo_root}/build"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build)
            build_first=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            echo "unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
        *)
            build_dir="$1"
            shift
            ;;
    esac
done

if [[ "${build_dir}" != /* ]]; then
    build_dir="${repo_root}/${build_dir}"
fi

examples=(
    01_readme_minimal
    02_score_matrix
    03_colormap_gallery
    04_missing_data_mask
    05_outliers_and_infinities
    06_crop_debug_chip
    07_covariance_coolwarm
    08_correlation_matrix
    09_confusion_matrix
    10_activation_heatmap
    11_residuals_heatmap
    12_matlab_peaks
    13_gaussian_blobs
    14_mandelbrot_preview
    15_false_color_rgb
    16_ppm_cat
    17_rgb_accessor_field
    18_capture_for_logs
    19_column_major_layout
    20_terminal_fit_modes
)

if [[ "${build_first}" -eq 1 ]]; then
    cmake -S "${repo_root}" -B "${build_dir}"
    cmake --build "${build_dir}"
fi

for example in "${examples[@]}"; do
    exe="${build_dir}/${example}"
    if [[ ! -x "${exe}" ]]; then
        echo "missing executable: ${exe}" >&2
        echo "try: $(basename "$0") --build ${build_dir}" >&2
        exit 1
    fi
done

for example in "${examples[@]}"; do
    exe="${build_dir}/${example}"
    printf '\n=== %s ===\n\n' "${example}"
    "${exe}"
done
