#!/bin/bash
# Skeleton: Visualization array task (eighth-wedge)
# - Reads the Nth parameter set from params_list.txt
# - Invokes a redacted Python visualization entry point
# - Uses placeholders for environment-specific settings

# ---------------- SLURM header (placeholders) ----------------
#SBATCH --job-name=viz_eighth_array
# SBATCH --account=<YOUR_ACCOUNT>
# SBATCH --partition=<YOUR_PARTITION>
#SBATCH --time=01:00:00
#SBATCH --mem=8G
#SBATCH --ntasks=1
#SBATCH --output=/dev/null
#SBATCH --error=logs/%x_%A_%a.err

set -euo pipefail

# --- Dry-run guard (set DRY_SIM=1 to echo & exit) ---
if [[ "${DRY_SIM:-0}" == "1" ]]; then
  echo "[DRY] file=$(basename "$0") task=$SLURM_ARRAY_TASK_ID"
  echo "[DRY] $(sed -n "$((SLURM_ARRAY_TASK_ID + 1))p" params_list.txt)"
  exit 0
fi

# ---------------- Environment (placeholder) ----------------
# module purge
# module load <python-module>         # or: source <your-venv>/bin/activate

# ---------------- Inputs ----------------
PARAMS_FILE="params_list.txt"
[[ -f "$PARAMS_FILE" ]] || { echo "[ERROR] Missing $PARAMS_FILE"; exit 1; }

# Expected fields (illustrative): tf d x0 y0 nsteps nreals nbins
read -r tf d x0 y0 nsteps nreals nbins < <(sed -n "$((SLURM_ARRAY_TASK_ID + 1))p" "$PARAMS_FILE")
[[ -n "${tf:-}" && -n "${nbins:-}" ]] || { echo "[ERROR] No params for task ${SLURM_ARRAY_TASK_ID}"; exit 1; }

mkdir -p logs

# ---------------- Run visualization (placeholder entry point) ----------------
# Use srun if preferred on your cluster.
# srun python3 visualize_eighth.py --tf "$tf" --d "$d" --x0 "$x0" --y0 "$y0" --nsteps "$nsteps" --nreals "$nreals" --nbins "$nbins"
python3 src/python/visualize_eighth.py \
  --tf "$tf" \
  --d "$d" \
  --x0 "$x0" \
  --y0 "$y0" \
  --nsteps "$nsteps" \
  --nreals "$nreals" \
  --nbins "$nbins"
