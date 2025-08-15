#!/bin/bash
# Skeleton: Eighth-wedge Monte Carlo array task
# - Reads the Nth parameter set from params_list.txt
# - Builds in a per-task directory
# - Compiles and runs the eighth-wedge Fortran solver
# - Writes results to a structured output path
#
# Environment-specific settings (account/partition/modules) are intentionally omitted.
# Fill these in locally if you want to execute on your cluster.

# ---------------- SLURM header (redacted placeholders) ----------------
#SBATCH --job-name=mc_eighth_array
# SBATCH --account=<YOUR_ACCOUNT>           # (optional) set in your environment
# SBATCH --partition=<YOUR_PARTITION>       # (optional)
#SBATCH --time=01:00:00
#SBATCH --mem=8G
#SBATCH --ntasks=1
#SBATCH --output=/dev/null                  # suppress normal stdout
#SBATCH --error=logs/%x_%A_%a.err           # one error log per array task

set -euo pipefail

# ---------------- Environment (placeholder) ----------------
# module purge
# module load <compiler-module>             # e.g., gcc or intel

# ---------------- Inputs ----------------
PARAMS_FILE="params_list.txt"
[[ -f "$PARAMS_FILE" ]] || { echo "[ERROR] Missing $PARAMS_FILE"; exit 1; }

# Expect one param set per line; adapt field names as needed.
# Example fields (illustrative only): tf d x0 y0 nsteps nreals nbins
read -r tf d x0 y0 nsteps nreals nbins < <(sed -n "$((SLURM_ARRAY_TASK_ID + 1))p" "$PARAMS_FILE")
[[ -n "${tf:-}" && -n "${nbins:-}" ]] || { echo "[ERROR] No params for task ${SLURM_ARRAY_TASK_ID}"; exit 1; }

# ---------------- Per-task build dir ----------------
BUILD_DIR="build_${SLURM_ARRAY_TASK_ID}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
rm -f *.o *.mod sim.exe fort.1

# ---------------- Task-local parameter module ----------------
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

# ---------------- Bring in sources (placeholder names/paths) ----------------
# Adjust filenames/paths to your local layout.
cp ../solver_eighth.f90 .           # redacted eighth-wedge solver (placeholder)
cp ../rng_mt19937.f90 .             # redacted RNG (placeholder)

# ---------------- Compile (flags intentionally omitted) ----------------
gfortran -c params.f90
gfortran -c rng_mt19937.f90
gfortran -c solver_eighth.f90
gfortran -o sim.exe solver_eighth.o rng_mt19937.o params.o

# ---------------- Output layout (generic, structure-only) ----------------
OUTDIR="../Eighth_Wedge/${nreals}_realizations/${nsteps}_time_steps/Initial_pos_X0_${x0}_Y0_${y0}/t_${tf}/d_${d}"
mkdir -p "$OUTDIR"
export OUTFILE="${OUTDIR}/results.txt"

# ---------------- Run ----------------
# srun ./sim.exe    # Uncomment if your cluster prefers srun
./sim.exe

# ---------------- Persist output if program wrote to fort.1 ----------------
if [[ -f fort.1 ]]; then
  mv -f fort.1 "$OUTFILE"
fi
