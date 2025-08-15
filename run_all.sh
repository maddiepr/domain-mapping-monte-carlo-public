#!/bin/bash
# --------------------------------------------------------------------------
# run_all.sh
# Orchestrates the 2D Monte Carlo PDE workflow:
# 1. Generate parameters
# 2. Submit simulation job array (quarter, eighth, or both)
# 3. Submit visualization jobs dependent on simulation completion
#
# Usage:
#   ./run_all.sh quarter|eighth|both [--dry-run] [--params FILE] [--preview N]
#
# Notes:
# - Environment-specific SLURM settings are inside MC_*.slurm and visualize_*.slurm
# - No sensitive information is included here
# --------------------------------------------------------------------------

set -eur pipefail   # stop on errors, unset vars, or failed pips

# Default paths and flags
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
ROOT_DIR="${SCRIPT_DIR}"  # adjust if you place this elsewhere
PARAMS_FILE="${ROOT_DIR}/params_list.txt"
DRY_RUN=0
PREVIEW_LINES=0

# Helpers for consistent output and dry-run support
info()  { echo "[INFO] $*"; }
warn()  { echo "[WARN] $*" >&2; }
error() { echo "[ERROR] $*" >&2; exit 1; }

# --- Parse arguments ---
[[ $# -ge 1 ]] || { echo "Usage: $0 quarter|eighth|both [...]"; exit 1; }
GEOMETRY="$1"; shift || true
case "$GEOMETRY" in quarter|eighth|both) ;; *) error "Geometry must be quarter, eighth, or both" ;; esac
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dry-run) DRY_RUN=1; shift ;;
    --params) PARAMS_FILE="$2"; shift 2 ;;
    --preview) PREVIEW_LINES="${2:-5}"; shift 2 ;;
    -h|--help) sed -n '1,20p' "$0"; exit 0 ;;
    *) error "Unknown option: $1" ;;
  esac
done

# --- Provenance info ---
GIT_REV="(no git)"
if command -v git >/dev/null && git -C "$ROOT_DIR" rev-parse --is-inside-work-tree >/dev/null; then
  GIT_REV="$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo '(no git)')"
fi
info "Provenance: $(date) | host=$(hostname) | git=$GIT_REV"

# --- Generate params if default file is used ---
GEN_PARAMS="${ROOT_DIR}/generate_params.sh"
if [[ "$PARAMS_FILE" == "${ROOT_DIR}/params_list.txt" && -x "$GEN_PARAMS" ]]; then
  info "Generating parameter list..."
  run_or_echo "bash \"$GEN_PARAMS\""
fi

# Verify params file exists and has content
[[ -f "$PARAMS_FILE" ]] || error "Params file not found: $PARAMS_FILE"
NUM_LINES=$(wc -l < "$PARAMS_FILE" | tr -d ' ')
[[ "$NUM_LINES" -ge 1 ]] || error "Params file is empty"
info "Params: $PARAMS_FILE ($NUM_LINES jobs)"
[[ "$PREVIEW_LINES" -gt 0 ]] && head -n "$PREVIEW_LINES" "$PARAMS_FILE"

# Ensure logs directory exists
mkdir -p "${ROOT_DIR}/logs"

# --- Submit a single geometry's workflow ---
submit_pipeline() {
  local geom="$1"
  local mc_slurm="${ROOT_DIR}/MC_${geom}.slurm"
  local vis_slurm="${ROOT_DIR}/visualize_${geom}.slurm"
  [[ -f "$mc_slurm"  ]] || error "Missing SLURM script: $mc_slurm"
  [[ -f "$vis_slurm" ]] || error "Missing SLURM script: $vis_slurm"

  info "Submitting Monte Carlo array for '$geom'..."
  if [[ $DRY_RUN -eq 1 ]]; then
    echo "DRY-RUN: sbatch --array=0-$((NUM_LINES - 1)) $mc_slurm"
    echo "DRY-RUN: sbatch --dependency=afterok:<MC_JOB_ID> --array=0-$((NUM_LINES - 1)) $vis_slurm"
    return
  fi

  MC_OUT=$(sbatch --array=0-$((NUM_LINES - 1)) "$mc_slurm")
  MC_JOB_ID=$(awk '{print $4}' <<< "$MC_OUT")
  [[ -n "$MC_JOB_ID" ]] || error "Could not parse MC job ID"
  info "$MC_OUT"

  info "Submitting visualization (afterok:$MC_JOB_ID)..."
  run_or_echo "sbatch --dependency=afterok:$MC_JOB_ID --array=0-$((NUM_LINES - 1)) \"$vis_slurm\""
}

# --- Run the workflow ---
case "$GEOMETRY" in
  quarter) submit_pipeline "quarter" ;;
  eighth)  submit_pipeline "eighth" ;;
  both)    submit_pipeline "quarter"; submit_pipeline "eighth" ;;
esac

info "Workflow submitted for: $GEOMETRY"