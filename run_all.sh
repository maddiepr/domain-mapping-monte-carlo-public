#!/bin/bash
# -------------------------------------------------------------
# run_all.sh (skeleton)
# Orchestrate the 2D Monte Carlo PDE workflow:
#   1) Generate parameter list
#   2) Submit MC array (quarter|eighth)
#   3) Submit visualization array with afterok dependency
#
# Usage:
#   ./run_all.sh quarter|eighth [--dry-run] [--preview N]
# -------------------------------------------------------------

# -------------------------------------------------------------
# Assumptions
# - Script is invoked from the repository root; working dir is fixed via sbatch --chdir.
# - Parameter file `params_list.txt` resides at repo root and contains seven whitespace-separated fields:
#   TF  D  X0  Y0  NSTEPS  NREALS  NBINS
# - Cluster-specific configuration (account/partition/modules/email) is defined inside:
#   scripts/slurm/MC_<geometry>.slurm and scripts/slurm/visualize_<geometry>.slurm
#
# Dependencies (relative to repo root)
# - generate_params.sh (executable)
# - scripts/slurm/MC_quarter.slurm, scripts/slurm/MC_eighth.slurm
# - scripts/slurm/visualize_quarter.slurm, scripts/slurm/visualize_eighth.slurm
#
# Flags
# - --dry-run   : print sbatch commands only (no submissions, no side effects)
# - --preview N : print the first N lines of params_list.txt for sanity-checking
#
# Exit behavior
# - Exits non-zero if required files are missing, if params_list.txt is empty,
#   or if submission fails; otherwise exits 0 after successful submissions.
#
# Security/PII
# - This is a public-safe skeleton: avoid usernames, emails, absolute paths, or account names here.
#   Keep any environment-specific details in the .slurm templates.
# -------------------------------------------------------------

set -euo pipefail

usage() { echo "Usage: $0 quarter|eighth [--dry-run] [--preview N]"; exit 1; }

# --- Geometry (required) ---
[[ $# -ge 1 ]] || usage
GEOMETRY="$1"; shift || true
case "$GEOMETRY" in quarter|eighth) ;; -h|--help) usage ;; *) echo "[ERROR] geometry must be 'quarter' or 'eighth'"; usage ;; esac

# --- Optional flags ---
DRY_RUN=0
PREVIEW=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dry-run) DRY_RUN=1; shift ;;
    --preview) PREVIEW="${2:-5}"; shift 2 ;;
    -h|--help) usage ;;
    *) echo "[ERROR] Unknown option: $1"; usage ;;
  esac
done

echo "[INFO] Running workflow for: $GEOMETRY"
echo "[INFO] Provenance: $(date) | host=$(hostname)"
# GIT_REV=$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo "n/a"); echo "[INFO] git=$GIT_REV"  # optional

# --- Paths ---
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
ROOT_DIR="${SCRIPT_DIR}"                       # keep this file at repo root
PARAMS_FILE="${ROOT_DIR}/params_list.txt"
GEN_PARAMS="${ROOT_DIR}/generate_params.sh"
MC_SLURM="${ROOT_DIR}/scripts/slurm/MC_${GEOMETRY}.slurm"
VIS_SLURM="${ROOT_DIR}/scripts/slurm/visualize_${GEOMETRY}.slurm"

# --- Step 1: Generate parameters ---
# Generate or refresh the parameter sweep (idempotent). If generate_params.sh is not present
# or is intentionally skipped, the script will proceed using the existing params_list.txt.
if [[ -x "$GEN_PARAMS" ]]; then
  echo "[INFO] Generating parameter list..."
  bash "$GEN_PARAMS"
else
  echo "[WARN] $GEN_PARAMS not found or not executable; skipping generation."
fi

[[ -f "$PARAMS_FILE" ]] || { echo "[ERROR] Missing $PARAMS_FILE"; exit 1; }
num_lines=$(wc -l < "$PARAMS_FILE" | tr -d ' ')
[[ "$num_lines" -ge 1 ]] || { echo "[ERROR] $PARAMS_FILE has zero lines"; exit 1; }

echo "[INFO] Params: $PARAMS_FILE  (jobs: $num_lines)"
if [[ "$PREVIEW" -gt 0 ]]; then
  echo "[INFO] Preview (first $PREVIEW line(s)):"
  head -n "$PREVIEW" "$PARAMS_FILE" || true
fi

mkdir -p "${ROOT_DIR}/logs"

# --- Check SLURM templates ---
[[ -f "$MC_SLURM"  ]] || { echo "[ERROR] Not found: $MC_SLURM"; exit 1; }
[[ -f "$VIS_SLURM" ]] || { echo "[ERROR] Not found: $VIS_SLURM"; exit 1; }

# --- Dry-run (print commands only) ---
# Dry-run mode prints the exact sbatch commands that would be executed, including
# array ranges and dependency wiring (afterok:<MC_JOB_ID>). Use this to validate
# geometry selection, parameter counts, and submission order without touching the scheduler.
if [[ "$DRY_RUN" -eq 1 ]]; then
  echo "DRY-RUN: sbatch --chdir=\"$ROOT_DIR\" --array=0-$((num_lines - 1)) \"$MC_SLURM\""
  echo "DRY-RUN: sbatch --chdir=\"$ROOT_DIR\" --dependency=afterok:<MC_JOB_ID> --array=0-$((num_lines - 1)) \"$VIS_SLURM\""
  echo "[INFO] Dry-run complete."
  exit 0
fi

# --- Submit MC array ---
# Submit the Monte Carlo array. One array index corresponds to one line in params_list.txt.
# The .slurm script is responsible for reading SLURM_ARRAY_TASK_ID and mapping it to parameters.
echo "[INFO] Submitting MC ${GEOMETRY} array..."
MC_SUBMIT_OUT=$(sbatch --chdir="$ROOT_DIR" --array=0-$((num_lines - 1)) "$MC_SLURM")
echo "[INFO] $MC_SUBMIT_OUT"
MC_JOB_ID=$(awk '{print $4}' <<< "$MC_SUBMIT_OUT")
[[ -n "${MC_JOB_ID:-}" ]] || { echo "[ERROR] Could not parse MC job ID"; exit 1; }

# --- Submit visualization with dependency ---
# Submit the visualization array with an afterok dependency on the MC array job ID.
# This guarantees post-processing starts only after all simulation tasks complete successfully.
echo "[INFO] Submitting visualization (afterok:$MC_JOB_ID)..."
sbatch --chdir="$ROOT_DIR" --dependency=afterok:"$MC_JOB_ID" --array=0-$((num_lines - 1)) "$VIS_SLURM" >/dev/null

echo "[INFO] Workflow submitted successfully for $GEOMETRY."
