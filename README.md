OKRegex

OKRegex is a custom regex compiler and solver pipeline designed to convert extended regular expressions (ERE) into a system of Diophantine equations. It integrates a C++ parser, optimizer, and matcher with a symbolic Python solver for equation validation.

✨ Features

🧠 Parses extended regular expressions (with repetition, grouping, alternation, character classes)

📦 Converts regex expressions into structured Expr trees

🔬 Generates equation fragments (x_frag) and binary equations (b_eq) for alternation paths

📊 Transforms parse trees into Diophantine equations with variable constraints

🐍 Offloads equation solving to a Python script (solver.py) using symbolic math

✅ Verifies input matches by reconstructing valid parse paths and matching segments

🔍 Supports character classes like [:alpha:], [:digit:], \d, \w, and bracketed expressions [a-z], [^...], etc.

🏗 Repository Structure

├── src/
│   ├── main.cpp                # Entry point
│   ├── core.cpp/hpp           # AST structures and enums
│   ├── parse.cpp/hpp          # Regex parser and scanner
│   ├── ops.cpp/hpp            # Operator/link detection and validation
│   ├── frags.cpp/hpp          # Fragment and equation generation
│   ├── matching.cpp/hpp       # Input matching + optimization
│   ├── solver_interface.cpp/hpp  # Python subprocess wrapper + output parsing
├── inc/                       # Header files (if separated)
├── solver.py                  # Python equation solver
├── Makefile                   # Build automation
└── README.md                  # You're here

🥪 Requirements

-std=c++23

Python 3.6+

Python dependencies: (for solver.py)

pip install sympy

🔧 Build Instructions

make

This will compile all .cpp files in src/ and produce the binary regex_solver.

To clean the build:

make clean

🚀 Running the Program

./regex_solver

You will be prompted to enter:

A regular expression (e.g. ((a|bc|d{1,5})(e|fg|h{2,3})){4,6})

An input string to test against

The tool will:

Parse and display the expression tree

Show generated equations

Call the Python solver

Check input matching based on the solutions

📜 Example

Enter a regex: (a|b)+c
Enter input string: aac

Expression Tree:
  Expr: group="(a|b)+c", op="", link="", active=1
    Expr: group="(a|b)", op="+", link="", active=1
      ...
  eq: 1+b0+b1=3
  b_eq: {b0}+{b1}=1

🧠 Credits

Designed and developed by Oleg Kovalenko. Regex parsing logic is customized and not based on standard libraries.

📄 License

MIT License — feel free to use, modify, and contribute.