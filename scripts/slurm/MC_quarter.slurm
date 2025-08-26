#!/bin/bash
# Skeleton: Quarter-plane Monte Carlo array task
# Requires: params_list.txt at repo root; placeholder sources in src/fortran/

# ---------------- SLURM header (placeholders) ----------------
#SBATCH --job-name=mc_quarter_array
# SBATCH --account=<YOUR_ACCOUNT>
# SBATCH --partition=<YOUR_PARTITION>
#SBATCH --time=01:00:00
#SBATCH --mem=8G
#SBATCH --ntasks=1
#SBATCH --output=/dev/null
#SBATCH --error=logs/%x_%A_%a.err

set -euo pipefail

# --- DRY RUN: echo & exit if requested ---
if [[ "${DRY_SIM:-0}" == "1" ]]; then
  echo "[DRY] file=$(basename "$0") task=$SLURM_ARRAY_TASK_ID"
  echo "[DRY] $(sed -n "$((SLURM_ARRAY_TASK_ID + 1))p" params_list.txt)"
  exit 0
fi

# ---------------- Environment (placeholder) ----------------
# module purge
# module load <compiler-module>   # e.g., gcc

# ---------------- Inputs ----------------
PARAMS_FILE="params_list.txt"
[[ -f "$PARAMS_FILE" ]] || { echo "[ERROR] Missing $PARAMS_FILE"; exit 1; }

# Expected fields (illustrative): tf d x0 y0 nsteps nreals nbins
read -r tf d x0 y0 nsteps nreals nbins < <(sed -n "$((SLURM_ARRAY_TASK_ID + 1))p" "$PARAMS_FILE")
[[ -n "${tf:-}" && -n "${nbins:-}" ]] || { echo "[ERROR] No params for task ${SLURM_ARRAY_TASK_ID}"; exit 1; }

# ---------------- Build per array task ----------------
BUILD_DIR="build_${SLURM_ARRAY_TASK_ID}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
rm -f *.o *.mod sim.exe fort.2

# From build_* (under repo root), sources live at ../src/fortran/
cp ../src/fortran/solver_quarter.f90 .   # placeholder solver
cp ../src/fortran/rng_mt19937.f90   .    # placeholder RNG

# Task-local parameter module
cat > params.f90 <<EOF
module sim_params
  implicit none
  double precision, parameter :: TF = ${tf}
  double precision, parameter :: d  = ${d}
  double precision, parameter :: x0 = ${x0}
  double precision, parameter :: y0 = ${y0}
  integer,          parameter :: nsteps = ${nsteps}
  integer,          parameter :: nreals = ${nreals}
  integer,          parameter :: nbins  = ${nbins}
end module sim_params
EOF

# Compile (flags intentionally omitted in skeleton)
gfortran -c params.f90
gfortran -c rng_mt19937.f90
gfortran -c solver_quarter.f90
gfortran -o sim.exe solver_quarter.o rng_mt19937.o params.o

# ---------------- Output layout (structure-only) ----------------
OUTDIR="../Quarter_plane/${nreals}_realizations/${nsteps}_time_steps/Initial_pos_X0_${x0}_Y0_${y0}/t_${tf}/d_${d}"
mkdir -p "$OUTDIR"
export OUTFILE="${OUTDIR}/results.txt"

# ---------------- Run ----------------
# srun ./sim.exe    # uncomment if your cluster prefers srun
./sim.exe

# Persist output if program wrote to fort.2
[[ -f fort.2 ]] && mv -f fort.2 "$OUTFILE"
