# Contributing Guidelines

Thank you for your interest in this project! While this repository is primarily maintained as part of a personal portfolio, contributions, and feedback are welcome under the guidelines below.

---

## Project Overview

This repository presents a redacted version of a Monte Carlo simulation framework for solving PDEs in conformally mapped domains. Core numerical methods have been removed for IP protection, but the architecture, simulation flow, and domain-specific logic remain.

---

## Getting Started

To set up the project locally:

1. Clone the repository:
   ```bash
   git clone https://github.com/maddiepr/domain-mapping-monte-carlo.git
   cd domain-mapping-monte-carlo
   ```

2. (Optional) Create and activate a virtual environment:
    ```bash
    python -m venv .venv
    source .venv/bin/activate  # or .venv\Scripts\activate on Windows
    ```

3. Install dependencies:
    ```bash
    pip install -r requirements.txt
    ```
    
For Fortran components, ensure you have a working Fortran compiler (e.g., ```gfortran```), and follow any additional build instructions in the ```fortran/``` directory.

---

## How to Contribute

If you'd like to suggest changes or improvements:
- Fork the repository and create a feature branch:
- Make your changes in a clear, modular way
- Submit a pull request with a concise description of your contribution

Pull requests will be reviewed as time allows. Since this is a personal academic project, turnaround may vary.

---

## Code Style

- Python: Follow PEP8 conventions
- Fortran: Use modular layout and comment major subroutines
- Shell/SLURM: Document scripts with in-line comments

---

## Issues & Feedback

Use the Issues tab to:
- Report bugs (e.g., build errors, broken logic)
- Ask questions about the project structure or methodology
- Suggest enhancements or documentation improvements

--- 

## Notes for Employers & Reviewers

While the full source code is not published due to academic IP concerns, I'm happy to walk through the complete version upon request in an interview or private consersation.

Feel free to contact me via [LinkedIn](www.linkedin.com/in/madeline-preston) or GitHub issues for context or collaboration.

Thank you!









