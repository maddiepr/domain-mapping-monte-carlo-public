#!/bin/bash
# -----------------------------------------------------------
# generate_params.sh (skeleton)
# Produces a params_list.txt for SLURM array jobs.
# Each line is one simulation configuration.
#
# Defaults are illustrative only. Override via CLI as needed.
# Example:
#   bash generate_params.sh
#   bash generate_params.sh --TF "0.05 0.1" --D "1 5" --preview 3
# -----------------------------------------------------------

set -euo pipefail

# ----------------------------
# Default illustrative values
# ----------------------------
OUTFILE="params_list.txt"

# Final time, diffusion, paired initial positions (x0[i], y0[i]),
# time steps, realizations, histogram bins.
TF_vals=(0.05 0.10)
D_vals=(1.0 5.0)
X0_vals=(0.10 0.50)
Y0_vals=(0.10 0.50)     # paired by index with X0_vals
NSTEPS_vals=(500)
NREALS_vals=(1000 5000)
NBINS_vals=(50)

PREVIEW=0  # number of lines to preview (0 = none)

# ----------------------------
# Helpers
# ----------------------------
info()  { echo "[INFO] $*"; }
error() { echo "[ERROR] $*" >&2; exit 1; }

# Read a space-separated string into a bash array variable
set_array_from_string() {
  local var="$1"; shift
  local str="$*"
  # shellcheck disable=SC2206
  local arr=($str)
  eval "$var=(${arr[*]/#/'\"'} )"
}

# ----------------------------
# Parse CLI overrides
# ----------------------------
while [[ $# -gt 0 ]]; do
  case "$1" in
    --outfile) OUTFILE="$2"; shift 2 ;;
    --TF)      set_array_from_string TF_vals "$2"; shift 2 ;;
    --D)       set_array_from_string D_vals "$2"; shift 2 ;;
    --X0)      set_array_from_string X0_vals "$2"; shift 2 ;;
    --Y0)      set_array_from_string Y0_vals "$2"; shift 2 ;;
    --NSTEPS)  set_array_from_string NSTEPS_vals "$2"; shift 2 ;;
    --NREALS)  set_array_from_string NREALS_vals "$2"; shift 2 ;;
    --NBINS)   set_array_from_string NBINS_vals "$2"; shift 2 ;;
    --preview) PREVIEW="${2:-5}"; shift 2 ;;
    -h|--help)
      sed -n '1,40p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) error "Unknown option: $1" ;;
  esac
done

# ----------------------------
# Validate arrays
# ----------------------------
[[ ${#TF_vals[@]}      -ge 1 ]] || error "TF_vals is empty"
[[ ${#D_vals[@]}       -ge 1 ]] || error "D_vals is empty"
[[ ${#X0_vals[@]}      -ge 1 ]] || error "X0_vals is empty"
[[ ${#Y0_vals[@]}      -ge 1 ]] || error "Y0_vals is empty"
[[ ${#NSTEPS_vals[@]}  -ge 1 ]] || error "NSTEPS_vals is empty"
[[ ${#NREALS_vals[@]}  -ge 1 ]] || error "NREALS_vals is empty"
[[ ${#NBINS_vals[@]}   -ge 1 ]] || error "NBINS_vals is empty"
[[ ${#X0_vals[@]} -eq ${#Y0_vals[@]} ]] || error "X0/Y0 must have equal length"

# ----------------------------
# Generate combinations
# ----------------------------
> "$OUTFILE"  # truncate if exists

len_xy=${#X0_vals[@]}
count=0

for tf in "${TF_vals[@]}"; do
  for d in "${D_vals[@]}"; do
    for ((i = 0; i < len_xy; i++)); do
      x0=${X0_vals[$i]}
      y0=${Y0_vals[$i]}
      for nsteps in "${NSTEPS_vals[@]}"; do
        for nreals in "${NREALS_vals[@]}"; do
          for nbins in "${NBINS_vals[@]}"; do
            echo "$tf $d $x0 $y0 $nsteps $nreals $nbins" >> "$OUTFILE"
            ((count++))
          done
        done
      done
    done
  done
done

info "Wrote $count configurations to $OUTFILE (illustrative values)"

# ----------------------------
# Optional preview
# ----------------------------
if [[ "$PREVIEW" -gt 0 ]]; then
  info "Preview (first $PREVIEW line(s)):"
  head -n "$PREVIEW" "$OUTFILE" || true
fi
